// Copyright Verizon Media. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
// deploy command tests
// Author: bratseth

package cmd

import (
//    "archive/zip"
    "github.com/stretchr/testify/assert"
    "testing"
)

func TestDeployCommand(t *testing.T) {
	assert.Equal(t,
	             "\x1b[32mSuccess \n",
	             executeCommand(t, []string{"deploy", "testdata/application.zip"}))
    assert.Equal(t, "http://127.0.0.1:19071/application/v2/tenant/default/prepareandactivate", lastRequest.URL.String())
    assert.Equal(t, "application/zip", lastRequest.Header.Get("Content-Type"))
    assert.Equal(t, "POST", lastRequest.Method)
    var body = lastRequest.Body
    assert.NotNil(t, body)
    buf := make([]byte, 10)
    body.Read(buf)
    assert.Equal(t, "PK\x03\x04\x14\x00\b\b\b\x00", string(buf))
}
