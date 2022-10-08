#!/usr/bin/bash
echo "run client(input is randomly selected)"
./mul_client
echo "run Tangor"
../../src/tangor $1
echo "run verify"
./mul_verify