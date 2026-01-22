import { Injectable } from "@angular/core";

@Injectable({ providedIn: "root" })
export class ExampleService {
  private get api() {
    if (!window || !window.HostApi || !window.HostApi.example) {
      throw new Error("HostApi is not ready.");
    }
    return window.HostApi.example;
  }

  setStatus(status: string): Promise<void> {
    return this.api.setStatus(status);
  }

  echo(text: string): Promise<string> {
    return this.api.echo(text);
  }

  add(a: number, b: number): Promise<number> {
    return this.api.add(a, b);
  }

  registerEventHandler(eventName: "statusChanged", handler: (status: string) => void): void {
    this.api.registerEventHandler(eventName, handler);
  }

  removeEventHandler(eventName: "statusChanged", handler: (status: string) => void): void {
    this.api.removeEventHandler(eventName, handler);
  }
}
