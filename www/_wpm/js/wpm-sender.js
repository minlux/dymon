/**
 * Map to store pending requests: id → { resolve, reject, timeout }
 */
const pendingRequests = new Map();
// Single listener for all incoming messages
window.addEventListener("message", (event) => {
    const data = event.data;
    if (!data || typeof data !== "object" || !("id" in data))
        return;
    const pending = pendingRequests.get(data.id);
    if (pending) {
        clearTimeout(pending.timeout);
        pending.resolve(data);
        pendingRequests.delete(data.id);
    }
});
/**
 * Sends a WpmRequest via window.postMessage and returns a Promise<WpmResponse>.
 */
export function sendWpmRequest(request, targetWindow, targetOrigin = "*", timeoutMs = 5000) {
    return new Promise((resolve, reject) => {
        if (pendingRequests.has(request.id)) {
            return reject(new Error(`Request with id ${request.id} is already pending.`));
        }
        const timeout = window.setTimeout(() => {
            pendingRequests.delete(request.id);
            reject(new Error(`Timeout waiting for WpmResponse with id: ${request.id}`));
        }, timeoutMs);
        pendingRequests.set(request.id, { resolve, reject, timeout });
        targetWindow.postMessage(request, targetOrigin);
    });
}
