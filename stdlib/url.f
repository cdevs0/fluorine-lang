// URL component encoding helpers; these do not parse or validate complete URLs.
fn encode_component(value) { "Percent-encode a string for use as a URL component."; return url_encode(value); }
fn decode_component(value) { "Decode percent-encoded URL component text."; return url_decode(value); }
