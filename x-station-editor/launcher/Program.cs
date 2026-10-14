using System.Diagnostics;
using System.Net;
using System.Net.Sockets;
using System.Reflection;

var assembly = Assembly.GetExecutingAssembly();
var resources = assembly.GetManifestResourceNames().Where(n => n.StartsWith("site/")).ToDictionary(n => n[5..].Replace('\\', '/'), n => n);
if (!resources.ContainsKey("index.html")) { Console.Error.WriteLine("Web assets missing. Run npm run build first."); return; }
var builder = WebApplication.CreateSlimBuilder(args);
builder.Logging.ClearProviders();
// Prefer one stable origin so browser MIDI permissions survive subsequent launches.
int port = 18742;
var probe = new TcpListener(IPAddress.Loopback, port);
try { probe.Start(); } catch (SocketException) { port = 0; } finally { probe.Stop(); }
// Ordinary loopback socket: Windows URL ACL/admin setup is not needed.
builder.WebHost.ConfigureKestrel(server => server.Listen(IPAddress.Loopback, port));
var app = builder.Build();
app.Run(async context =>
{
    context.Response.Headers.XContentTypeOptions = "nosniff";
    context.Response.Headers.CacheControl = "no-store";
    context.Response.Headers["Permissions-Policy"] = "midi=(self)";
    context.Response.Headers["Content-Security-Policy"] = "default-src 'self'; script-src 'self'; style-src 'self' 'unsafe-inline'; img-src 'self' data:; object-src 'none'; frame-ancestors 'none'";
    // Exact allowlist of embedded resources, without filesystem path construction.
    string path = context.Request.Path.Value?.TrimStart('/') ?? "";
    if (path.Length == 0) path = "index.html";
    if (context.Request.Method is not ("GET" or "HEAD") || !resources.TryGetValue(path, out var resource))
    { context.Response.StatusCode = 404; return; }
    context.Response.ContentType = Path.GetExtension(path) switch
    {
        ".html" => "text/html; charset=utf-8", ".js" => "text/javascript; charset=utf-8",
        ".css" => "text/css; charset=utf-8", ".json" => "application/json", ".svg" => "image/svg+xml",
        ".png" => "image/png", ".woff2" => "font/woff2", _ => "application/octet-stream"
    };
    using var stream = assembly.GetManifestResourceStream(resource)!;
    context.Response.ContentLength = stream.Length;
    if (context.Request.Method != "HEAD") await stream.CopyToAsync(context.Response.Body);
});
await app.StartAsync();
string url = app.Urls.Single() + "/";
Console.WriteLine("X-Station Web Editor 0.1");
Console.WriteLine("Open in Google Chrome or Microsoft Edge: " + url);
Console.WriteLine("Keep this window open. Ctrl+C stops the editor. All files stay on this computer.");
if (!args.Contains("--no-browser"))
{
    try
    {
        string? edge = OperatingSystem.IsWindows() ? new[]
        {
            Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86), "Microsoft", "Edge", "Application", "msedge.exe"),
            Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles), "Microsoft", "Edge", "Application", "msedge.exe")
        }.FirstOrDefault(File.Exists) : null;
        Process.Start(edge is null ? new ProcessStartInfo(url) { UseShellExecute = true } : new ProcessStartInfo(edge, url) { UseShellExecute = true });
    }
    catch (Exception e) { Console.WriteLine("Open the address above manually: " + e.Message); }
}
await app.WaitForShutdownAsync();
