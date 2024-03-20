// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package mock

import (
	"bytes"
	"fmt"
	"io"
	"net/http"
	"strconv"
	"time"
)

type HTTPClient struct {
	// The responses to return for future requests. Once a response is consumed, it's removed from this slice.
	nextResponses []HTTPResponse

	// The errors to return for future requests. If non-nil, these errors are returned before any responses in nextResponses.
	nextErrors []error

	// LastRequest is the last HTTP request made through this.
	LastRequest *http.Request

	// ReadBody controls whether the client consumes the request body automatically. If true, LastBody will contain the
	// body of the most recent request.
	ReadBody bool

	// LastBody is a copy of the last request payload sent through this.
	LastBody []byte

	// Requests contains all requests made through this.
	Requests []*http.Request
}

type HTTPResponse struct {
	URI    string
	Status int
	Body   []byte
	Header http.Header
}

func (c *HTTPClient) NextStatus(status int) { c.NextResponseBytes(status, nil) }

func (c *HTTPClient) NextResponseString(status int, body string) {
	c.NextResponseBytes(status, []byte(body))
}

func (c *HTTPClient) NextResponseBytes(status int, body []byte) {
	c.nextResponses = append(c.nextResponses, HTTPResponse{Status: status, Body: body})
}

func (c *HTTPClient) NextResponse(response HTTPResponse) {
	c.nextResponses = append(c.nextResponses, response)
}

func (c *HTTPClient) NextResponseError(err error) {
	c.nextErrors = append(c.nextErrors, err)
}

func (c *HTTPClient) Do(request *http.Request, timeout time.Duration) (*http.Response, error) {
	c.LastRequest = request
	if len(c.nextErrors) > 0 {
		err := c.nextErrors[0]
		c.nextErrors = c.nextErrors[1:]
		return nil, err
	}
	response := HTTPResponse{Status: 200}
	if len(c.nextResponses) > 0 {
		response = c.nextResponses[0]
		if response.URI != "" && response.URI != request.URL.RequestURI() {
			return nil, fmt.Errorf("uri of response is %s, which does not match request uri %s", response.URI, request.URL.RequestURI())
		}
		c.nextResponses = c.nextResponses[1:]
	}
	if c.ReadBody && request.Body != nil {
		body, err := io.ReadAll(request.Body)
		if err != nil {
			return nil, err
		}
		c.LastBody = body
	} else {
		c.LastBody = nil
	}
	c.Requests = append(c.Requests, request)
	if response.Header == nil {
		response.Header = make(http.Header)
	}
	return &http.Response{
			Status:     "Status " + strconv.Itoa(response.Status),
			StatusCode: response.Status,
			Body:       io.NopCloser(bytes.NewBuffer(response.Body)),
			Header:     response.Header,
		},
		nil
}
