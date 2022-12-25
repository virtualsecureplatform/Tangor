#!/usr/bin/bash
echo "run client(input is randomly selected)"
./addermini_client
echo "run Tangor"
../../src/tangor $1
echo "run verify"
./addermini_verify