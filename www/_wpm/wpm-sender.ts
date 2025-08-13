import type { WpmRequest, WpmResponse } from "./wpm"; // import the types

/**
 * Map to store pending requests: id → { resolve, reject, timeout }
 */
const pendingRequests = new Map<string, { resolve: (res: WpmResponse) => void; reject: (err: any) => void; timeout: number }>();

// Single listener for all incoming messages
window.addEventListener("message", (event: MessageEvent) => {
  const data = event.data;
  if (!data || typeof data !== "object" || !("id" in data)) return;

  const pending = pendingRequests.get(data.id);
  if (pending) {
    clearTimeout(pending.timeout);
    pending.resolve(data as WpmResponse);
    pendingRequests.delete(data.id);
  }
});

/**
 * Sends a WpmRequest via window.postMessage and returns a Promise<WpmResponse>.
 */
export function sendWpmRequest(
  request: WpmRequest,
  targetWindow: Window,
  targetOrigin: string = "*",
  timeoutMs: number = 5000
): Promise<WpmResponse> {
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
