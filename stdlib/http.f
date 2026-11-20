
class Http {
    fn get(url) {
        return http_get(url);
    }

    fn get_with(url, headers, options) {
        return http_get(url, headers, options);
    }

    fn request(method, url, headers, body, options) {
        return http_request(method, url, headers, body, options);
    }

    fn post(url, headers, body, options) {
        return http_request("POST", url, headers, body, options);
    }

    fn put(url, headers, body, options) {
        return http_request("PUT", url, headers, body, options);
    }

    fn delete(url, headers, options) {
        return http_request("DELETE", url, headers, "", options);
    }
}
