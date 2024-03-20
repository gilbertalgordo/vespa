// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package document

import (
	"bytes"
	"fmt"
	"io"
	"math"
	"net/http"
	"net/url"
	"runtime"
	"strconv"
	"strings"
	"sync"
	"sync/atomic"
	"time"

	"github.com/go-json-experiment/json"
	"github.com/klauspost/compress/gzip"

	"github.com/vespa-engine/vespa/client/go/internal/httputil"
)

type Compression int

const (
	CompressionAuto Compression = iota
	CompressionNone
	CompressionGzip
)

// Client represents a HTTP client for the /document/v1/ API.
type Client struct {
	options     ClientOptions
	httpClients []*countingHTTPClient
	now         func() time.Time
	sendCount   atomic.Int32
	gzippers    sync.Pool
	buffers     sync.Pool
	pending     chan *pendingDocument
}

// ClientOptions specifices the configuration options of a feed client.
type ClientOptions struct {
	BaseURL     string
	Timeout     time.Duration
	Route       string
	TraceLevel  int
	Compression Compression
	Speedtest   bool
	NowFunc     func() time.Time
}

type countingHTTPClient struct {
	client   httputil.Client
	inflight atomic.Int64
}

func (c *countingHTTPClient) Do(req *http.Request, timeout time.Duration) (*http.Response, error) {
	defer c.inflight.Add(-1)
	return c.client.Do(req, timeout)
}

type pendingDocument struct {
	document Document
	prepared chan bool

	request *http.Request
	buf     *bytes.Buffer
	err     error
}

func NewClient(options ClientOptions, httpClients []httputil.Client) (*Client, error) {
	if len(httpClients) < 1 {
		return nil, fmt.Errorf("need at least one HTTP client")
	}
	_, err := url.Parse(options.BaseURL)
	if err != nil {
		return nil, fmt.Errorf("invalid base url: %w", err)
	}
	countingClients := make([]*countingHTTPClient, 0, len(httpClients))
	for _, client := range httpClients {
		countingClients = append(countingClients, &countingHTTPClient{client: client})
	}
	nowFunc := options.NowFunc
	if nowFunc == nil {
		nowFunc = time.Now
	}
	c := &Client{
		options:     options,
		httpClients: countingClients,
		now:         nowFunc,
		pending:     make(chan *pendingDocument, 4096),
	}
	c.gzippers.New = func() any { return gzip.NewWriter(io.Discard) }
	c.buffers.New = func() any { return &bytes.Buffer{} }
	for i := 0; i < runtime.NumCPU(); i++ {
		go c.preparePending()
	}
	return c, nil
}

func writeQueryParam(sb *bytes.Buffer, start int, escape bool, k, v string) {
	if sb.Len() == start {
		sb.WriteString("?")
	} else {
		sb.WriteString("&")
	}
	sb.WriteString(k)
	sb.WriteString("=")
	if escape {
		sb.WriteString(url.QueryEscape(v))
	} else {
		sb.WriteString(v)
	}
}

func (c *Client) writeDocumentPath(id Id, sb *bytes.Buffer) {
	sb.WriteString(strings.TrimSuffix(c.options.BaseURL, "/"))
	sb.WriteString("/document/v1/")
	sb.WriteString(url.PathEscape(id.Namespace))
	sb.WriteString("/")
	sb.WriteString(url.PathEscape(id.Type))
	if id.Number != nil {
		sb.WriteString("/number/")
		n := uint64(*id.Number)
		sb.WriteString(strconv.FormatUint(n, 10))
	} else if id.Group != "" {
		sb.WriteString("/group/")
		sb.WriteString(url.PathEscape(id.Group))
	} else {
		sb.WriteString("/docid")
	}
	sb.WriteString("/")
	sb.WriteString(url.PathEscape(id.UserSpecific))
}

func (c *Client) methodAndURL(d Document, sb *bytes.Buffer) (string, string) {
	httpMethod := ""
	switch d.Operation {
	case OperationPut:
		httpMethod = http.MethodPost
	case OperationUpdate:
		httpMethod = http.MethodPut
	case OperationRemove:
		httpMethod = http.MethodDelete
	}
	// Base URL and path
	c.writeDocumentPath(d.Id, sb)
	// Query part
	queryStart := sb.Len()
	if c.options.Timeout > 0 {
		writeQueryParam(sb, queryStart, false, "timeout", strconv.FormatInt(c.options.Timeout.Milliseconds(), 10)+"ms")
	}
	if c.options.Route != "" {
		writeQueryParam(sb, queryStart, true, "route", c.options.Route)
	}
	if c.options.TraceLevel > 0 {
		writeQueryParam(sb, queryStart, false, "tracelevel", strconv.Itoa(c.options.TraceLevel))
	}
	if c.options.Speedtest {
		writeQueryParam(sb, queryStart, false, "dryRun", "true")
	}
	if d.Condition != "" {
		writeQueryParam(sb, queryStart, true, "condition", d.Condition)
	}
	if d.Create {
		writeQueryParam(sb, queryStart, false, "create", "true")
	}
	return httpMethod, sb.String()
}

func (c *Client) leastBusyClient() *countingHTTPClient {
	leastBusy := c.httpClients[0]
	min := int64(math.MaxInt64)
	next := c.sendCount.Add(1)
	start := int(next) % len(c.httpClients)
	for i := range c.httpClients {
		j := (i + start) % len(c.httpClients)
		client := c.httpClients[j]
		inflight := client.inflight.Load()
		if inflight < min {
			leastBusy = client
			min = inflight
		}
	}
	leastBusy.inflight.Add(1)
	return leastBusy
}

func (c *Client) gzipWriter(w io.Writer) *gzip.Writer {
	gzipWriter := c.gzippers.Get().(*gzip.Writer)
	gzipWriter.Reset(w)
	return gzipWriter
}

func (c *Client) buffer() *bytes.Buffer {
	buf := c.buffers.Get().(*bytes.Buffer)
	buf.Reset()
	return buf
}

func (c *Client) preparePending() {
	for pd := range c.pending {
		pd.buf = c.buffer()
		method, url := c.methodAndURL(pd.document, pd.buf)
		pd.request, pd.err = c.createRequest(method, url, pd.document.Body, pd.buf)
		pd.prepared <- true
	}
}

func (c *Client) prepare(document Document) (*http.Request, *bytes.Buffer, error) {
	pd := pendingDocument{document: document, prepared: make(chan bool)}
	c.pending <- &pd
	<-pd.prepared
	return pd.request, pd.buf, pd.err
}

func newRequest(method, url string, body io.Reader, gzipped bool) (*http.Request, error) {
	req, err := http.NewRequest(method, url, body)
	if err != nil {
		return nil, err
	}
	req.Header.Set("Content-Type", "application/json; charset=utf-8")
	if gzipped {
		req.Header.Set("Content-Encoding", "gzip")
	}
	return req, nil
}

func (c *Client) createRequest(method, url string, body []byte, buf *bytes.Buffer) (*http.Request, error) {
	buf.Reset()
	if len(body) == 0 {
		return newRequest(method, url, nil, false)
	}
	useGzip := c.options.Compression == CompressionGzip || (c.options.Compression == CompressionAuto && len(body) > 512)
	var r io.Reader
	if useGzip {
		buf.Grow(min(1024, len(body)))
		zw := c.gzipWriter(buf)
		defer c.gzippers.Put(zw)
		if _, err := zw.Write(body); err != nil {
			return nil, err
		}
		if err := zw.Close(); err != nil {
			return nil, err
		}
		r = buf
	} else {
		r = bytes.NewReader(body)
	}
	return newRequest(method, url, r, useGzip)
}

func (c *Client) clientTimeout() time.Duration {
	if c.options.Timeout < 1 {
		return 190 * time.Second
	}
	return c.options.Timeout*11/10 + 1000 // slightly higher than the server-side timeout
}

// Send given document to the endpoint configured in this client.
func (c *Client) Send(document Document) Result {
	start := c.now()
	result := Result{Id: document.Id}
	req, buf, err := c.prepare(document)
	defer c.buffers.Put(buf)
	if err != nil {
		return resultWithErr(result, err, 0)
	}
	bodySize := len(document.Body)
	if buf.Len() > 0 {
		bodySize = buf.Len()
	}
	resp, err := c.leastBusyClient().Do(req, c.clientTimeout())
	elapsed := c.now().Sub(start)
	if err != nil {
		return resultWithErr(result, err, elapsed)
	}
	defer resp.Body.Close()
	return c.resultWithResponse(resp, bodySize, result, elapsed, buf, false)
}

// Get retrieves document with given ID.
func (c *Client) Get(id Id) Result {
	start := c.now()
	buf := c.buffer()
	defer c.buffers.Put(buf)
	c.writeDocumentPath(id, buf)
	url := buf.String()
	result := Result{Id: id}
	req, err := http.NewRequest(http.MethodGet, url, nil)
	if err != nil {
		return resultWithErr(result, err, 0)
	}
	resp, err := c.leastBusyClient().Do(req, c.clientTimeout())
	elapsed := c.now().Sub(start)
	if err != nil {
		return resultWithErr(result, err, elapsed)
	}
	defer resp.Body.Close()
	return c.resultWithResponse(resp, 0, result, elapsed, buf, true)
}

func resultWithErr(result Result, err error, elapsed time.Duration) Result {
	result.Status = StatusTransportFailure
	result.Err = err
	result.Latency = elapsed
	return result
}

func (c *Client) resultWithResponse(resp *http.Response, sentBytes int, result Result, elapsed time.Duration, buf *bytes.Buffer, copyBody bool) Result {
	result.HTTPStatus = resp.StatusCode
	switch resp.StatusCode {
	case 200:
		result.Status = StatusSuccess
	case 412:
		result.Status = StatusConditionNotMet
	case 502, 504, 507:
		result.Status = StatusVespaFailure
	default:
		result.Status = StatusTransportFailure
	}
	buf.Reset()
	written, err := io.Copy(buf, resp.Body)
	if err != nil {
		result = resultWithErr(result, err, elapsed)
	} else {
		if result.Success() && c.options.TraceLevel > 0 {
			var jsonResponse struct {
				Trace json.RawValue `json:"trace"`
			}
			if err := json.Unmarshal(buf.Bytes(), &jsonResponse); err != nil {
				result = resultWithErr(result, fmt.Errorf("failed to decode json response: %w", err), elapsed)
			} else {
				result.Trace = string(jsonResponse.Trace)
			}
		}
		if !result.Success() || copyBody {
			result.Body = make([]byte, buf.Len())
			copy(result.Body, buf.Bytes())
		}
	}
	result.Latency = elapsed
	result.BytesSent = int64(sentBytes)
	result.BytesRecv = int64(written)
	return result
}
