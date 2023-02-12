# otel

libMetrics

A simple library that tries to make some sense of CNCF OpenTelemetry

### Testing 

- a begging of series of tests in an experimental playground using GTest

### Trace

- A client and server distributed trace, does not use current span or carriers as it is designed to transport context across non standard communications.

the client connects to 3333 , 3334 , 3335 repeatadly passing the context of a distributed state.

to run ./client

- A Server that responds to the client

./server 3333&
./server 3334&
./server 3335&

It is currently configured to send to port 4317 using OTLPHTTP

pick it up with a standard collector 

view it as you please.