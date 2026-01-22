import { Component, OnInit } from "@angular/core";
import { ExampleService } from "../generated/ExampleService";

@Component({
  selector: "app-root",
  templateUrl: "./app.component.html",
  styleUrls: ["./app.component.css"]
})
export class AppComponent implements OnInit {
  status = "Waiting for HostApi...";
  logLines: string[] = [];
  ready = false;
  statusInput = "";

  constructor(private readonly exampleApi: ExampleService) {}

  ngOnInit(): void {
    this.attachHostApi();
  }

  sendData(): void {
    if (!this.ready) {
      this.log("HostApi not ready.");
      return;
    }
    window.HostApi?.sendData({ clicked: "sendData", at: new Date().toISOString() });
    this.log("sendData() invoked");
  }

  setOutput(): void {
    if (!this.ready) {
      this.log("HostApi not ready.");
      return;
    }
    window.HostApi?.setOutput("Angular output set at " + new Date().toLocaleTimeString());
    this.log("setOutput() invoked");
  }

  async getInput(): Promise<void> {
    if (!this.ready) {
      this.log("HostApi not ready.");
      return;
    }
    this.log("getInput() waiting...");
    const value = await window.HostApi?.getInput();
    this.log("getInput() resolved: " + value);
  }

  async exampleEcho(): Promise<void> {
    if (!this.ready || !window.HostApi?.example) {
      this.log("example API not available.");
      return;
    }
    const value = await this.exampleApi.echo("Hello from Angular");
    this.log("example.echo() => " + value);
  }

  async exampleStatus(): Promise<void> {
    if (!this.ready || !window.HostApi?.example) {
      this.log("example API not available.");
      return;
    }
    const status = this.statusInput.trim().length
      ? this.statusInput.trim()
      : "Angular status @ " + new Date().toLocaleTimeString();
    await this.exampleApi.setStatus(status);
    this.log("example.setStatus() sent");
  }

  private attachHostApi(): void {
    const onReady = () => {
      if (!window.HostApi) {
        this.status = "HostApi not available.";
        return;
      }
      this.ready = true;
      this.status = "HostApi ready";
      if (window.HostApi.version) {
        this.log("HostApi version: " + window.HostApi.version);
      }
      if (Array.isArray(window.HostApi.validEventTypes)) {
        this.log("Valid events: " + window.HostApi.validEventTypes.join(", "));
      }

      if (window.HostApi.example) {
        this.exampleApi.registerEventHandler("statusChanged", (status: string) => {
          this.log("statusChanged: " + status);
        });
      }
    };

    if (window.HostApi) {
      onReady();
    } else {
      this.status = "Waiting for HostApiReady...";
      window.addEventListener("HostApiReady", onReady, { once: true });
    }

    window.addEventListener("HostApiVersionMismatch", (evt: Event) => {
      const detail = (evt as CustomEvent).detail || {};
      this.log("Version mismatch: expected " + detail.expected + ", actual " + detail.actual);
    });
  }

  private log(line: string): void {
    this.logLines = [...this.logLines, line];
  }
}
