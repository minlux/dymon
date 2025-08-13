export type Method = "PUT" | "POST";

export interface WpmRequest {
  id: string; //initialize this wit crypto.randomUUID()
  method: Method;
  url: string;
  params?: URLSearchParams;
  headers?: HeadersInit;
  body?: BodyInit;
}

export interface WpmResponse {
  id: string;
  status: number;
  statusText?: string;
  headers?: HeadersInit;
  body?: BodyInit;
}
