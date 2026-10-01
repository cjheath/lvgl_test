# wifi_scanner

A `Thread` that owns the WiFi driver and scans for access points. No other
thread calls `esp_wifi_*`; you talk to the scanner over `MessageQueue`s.

    MessageQueue    inbox;                       // where replies arrive
    WifiScanner scanner(inbox);           // starts the thread

    VariantArray request;
    request.push(Variant("scan"));
    scanner.requests.push(Variant(request));   // wrap it: push(VariantArray) sends each element separately

    Variant reply = inbox.pop();

## Requests

Push a `Variant` holding a `VariantArray` onto `scanner.requests`:

- `["scan"]` - scan now.
- `["auto", ms]` - scan again whenever `ms` milliseconds pass with no
  request. `0` stops it.
- `["quit"]` - stop the WiFi driver and end the thread. You can then `join()`.

## Replies

Each reply is a `VariantArray` whose first element names it:

- `["ready"]` - the driver started. You get this once, first.
- `["scan", [ap, ...]]` - the access points found, strongest first. Each
  `ap` is `[ssid, rssi, channel, auth, bssid]`: a string, an integer in
  dBm, an integer, one of `open`, `WEP`, `WPA`, `WPA2`, `WPA/WPA2`, `WPA3`,
  `WPA2/WPA3`, `OWE` or `other`, and a string like `aa:bb:cc:dd:ee:ff`.
- `["error", text]` - a request or the driver failed. If the driver failed
  to start, the thread then ends.
- `["quit"]` - the thread is about to end.

At most 40 access points are returned.
