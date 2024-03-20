// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package document

import (
	"reflect"
	"testing"
	"time"
)

func TestStatsAdd(t *testing.T) {
	var stats Stats
	stats.Add(Result{HTTPStatus: 200, Latency: 200 * time.Millisecond}, false)
	stats.Add(Result{HTTPStatus: 200, Latency: 400 * time.Millisecond}, false)
	stats.Add(Result{HTTPStatus: 200, Latency: 100 * time.Millisecond}, false)
	stats.Add(Result{HTTPStatus: 200, Latency: 500 * time.Millisecond}, false)
	stats.Add(Result{HTTPStatus: 200, Latency: 300 * time.Millisecond}, false)
	stats.Add(Result{HTTPStatus: 500, Latency: 100 * time.Millisecond}, false)
	stats.Add(Result{HTTPStatus: 200, Latency: 100 * time.Millisecond}, true)
	expected := Stats{
		Operations:      6,
		Requests:        7,
		Responses:       7,
		ResponsesByCode: map[int]int64{200: 6, 500: 1},
		TotalLatency:    1700 * time.Millisecond,
		MinLatency:      100 * time.Millisecond,
		MaxLatency:      500 * time.Millisecond,
	}
	if !reflect.DeepEqual(stats, expected) {
		t.Errorf("got %+v, want %+v", stats, expected)
	}
	if want, got := int64(1), stats.Unsuccessful(); want != got {
		t.Errorf("got stats.Unsuccessful() = %d, want %d", got, want)
	}
}

func TestStatsClone(t *testing.T) {
	var a Stats
	a.Add(Result{HTTPStatus: 200}, false)
	b := a.Clone()
	a.Add(Result{HTTPStatus: 200}, false)

	want := Stats{Operations: 1, Requests: 1, Responses: 1, ResponsesByCode: map[int]int64{200: 1}}
	if !reflect.DeepEqual(b, want) {
		t.Errorf("got %+v, want %+v", b, want)
	}
}
