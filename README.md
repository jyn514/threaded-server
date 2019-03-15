# crappy-server
Returns 204 No Response to every HTTP request.
Hardcoded to listen to port 8080.
Will probably crash if you try anything fancy.

However, it handles requests at about 10 msec/response, which is comparable to Nginx.
Try `ab -n 1000 -c 100 localhost:8080/` for a comparison.

## But Why?
I'm taking networking and operating systems courses where we don't write any code.
I thought it would be nice to get hands-on in system internals.

## TODO
- Customize ports
- Serve static files
- Figure out why -fsanitize=thread thinks my interrupt handler isn't safe
- Return headers (Server, Date, etc.)
- Check what happens if we run into the thread limit
(currently, we spawn a thread for every request no matter how many threads we already have)
- Figure out why `sleep(10)` in `handle_request` adds a response time of 18 seconds
- Logging (?)
- Dynamic pages (??)
- HTTP/2.0 (???)
