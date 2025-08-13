// Listen for all postMessage events
window.addEventListener("message", async (event) => {
    const data = event.data;
    // Validate that it looks like a WpmRequest
    if (!data || typeof data !== "object" || !("id" in data) || !("method" in data) || !("url" in data)) {
        return;
    }
    const request = data;
    console.log("Received WpmRequest:", request);
    try {
        // Build the fetch options
        const fetchOptions = {
            method: request.method,
            headers: request.headers,
            body: request.body,
        };
        // Append params to URL if present
        let url = request.url;
        if (request.params) {
          url = request.url + '?' + new URLSearchParams(request.params).toString();
        }
        // Perform the HTTP request
        const fetchResponse = await fetch(url, fetchOptions);
        // Read the response body as a Blob
        const responseBody = await fetchResponse.blob();
        // Construct the WpmResponse
        const response = {
            id: request.id,
            status: fetchResponse.status,
            statusText: fetchResponse.statusText,
            headers: JSON.parse(JSON.stringify(fetchResponse.headers)),
            body: responseBody,
        };
        // Send response back to the source window
        event.source?.postMessage(response, event.origin);
    }
    catch (err) {
        // On fetch error, send a failure response
        const response = {
            id: request.id,
            status: 500,
            statusText: err.message,
        };
        event.source?.postMessage(response, event.origin);
    }
});
export {};
