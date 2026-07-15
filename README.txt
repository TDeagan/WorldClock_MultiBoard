World Clock v4.4.4 Apply-link fix

Replace:
  config.h
  62_RuntimeWebConfig.ino

The Apply Settings control no longer submits an HTML form.

It is now an ordinary link, matching the controls that are known to reach
the ESP32 successfully. A small inline function reads the visible controls,
constructs a GET query, and navigates directly to /apply-settings.

The link also has a fallback URL. Therefore, clicking Apply must produce one
of these results:

1. Normal operation:
     Runtime web: GET /apply-settings; ...
     ...
     source=apply-link

2. If the browser refuses to execute the inline page function:
     Runtime web: GET /apply-settings; 1 argument(s)
     arg[0] source=fallback

   The page then displays:
     The Apply link reached the clock, but the browser did not supply the
     selected values.

If clicking Apply still produces no Serial output, verify Diagnostics shows
Firmware 4.4.4. With this version Apply is the same HTML element type as the
working Map Maintenance links.
