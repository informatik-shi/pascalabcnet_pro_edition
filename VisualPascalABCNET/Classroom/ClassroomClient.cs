using System;
using System.Collections.Generic;
using System.IO;
using System.Net;
using System.Net.Http;
using System.Text;
using System.Threading.Tasks;
using System.Web.Script.Serialization;

namespace VisualPascalABC.Classroom
{
    internal sealed class ClassroomClient : IDisposable
    {
        private readonly HttpClient http = new HttpClient { Timeout = TimeSpan.FromSeconds(12) };
        private readonly JavaScriptSerializer json = new JavaScriptSerializer();
        private string token;
        private Uri baseUri;

        public bool IsLoggedIn { get { return token != null; } }
        public string Login { get; private set; }

        public async Task LoginAsync(string address, int port, bool https, string login, string password)
        {
            await LogoutAsync().ConfigureAwait(false);
            if (port < 1 || port > 65535 || string.IsNullOrWhiteSpace(address))
                throw new ArgumentException("Укажите IP-адрес и порт сервера");
            IPAddress parsed;
            if (!IPAddress.TryParse(address.Trim(), out parsed))
                throw new ArgumentException("Неверный IP-адрес сервера");
            var candidate = new UriBuilder(https ? "https" : "http", parsed.ToString(), port).Uri;
            var result = await PostAsync<LoginReply>(candidate, "api/login", new
            {
                login = login.Trim(),
                password
            }, false).ConfigureAwait(false);
            if (string.IsNullOrEmpty(result.token) || result.role != "student")
                throw new InvalidOperationException("Для входа в IDE нужна учётная запись ученика");
            baseUri = candidate;
            token = result.token;
            Login = result.login;
        }

        public async Task LogoutAsync()
        {
            if (IsLoggedIn)
            {
                try { await PostAsync<object>(baseUri, "api/logout", new { }, true).ConfigureAwait(false); }
                catch { /* A disconnected server must not prevent local logout. */ }
            }
            token = null;
            Login = null;
        }

        public Task HeartbeatAsync()
        {
            return PostAsync<object>(baseUri, "api/heartbeat", new { }, true);
        }

        public Task StartTaskAsync(string taskId)
        {
            return PostAsync<object>(baseUri, "api/tasks/" + taskId + "/start", new { }, true);
        }

        public async Task<List<ClassroomTest>> PublicTestsAsync(string taskId)
        {
            var result = await GetAsync<TestsReply>("api/tasks/" + taskId + "/tests")
                .ConfigureAwait(false);
            return result.tests ?? new List<ClassroomTest>();
        }

        public async Task<int> SubmitAsync(string taskId, string source, int passed, int total)
        {
            var result = await PostAsync<SubmissionReply>(baseUri, "api/submissions", new
            {
                task_id = taskId, source, local_passed = passed, local_total = total
            }, true).ConfigureAwait(false);
            return result.submission_id;
        }

        public Task<SubmissionStatus> SubmissionAsync(int id)
        {
            return GetAsync<SubmissionStatus>("api/submissions/" + id);
        }

        public async Task<byte[]> DownloadAssignmentsAsync()
        {
            using (var request = CreateRequest(HttpMethod.Get, "api/assignments.zip"))
            using (var response = await http.SendAsync(request).ConfigureAwait(false))
            {
                await EnsureSuccess(response).ConfigureAwait(false);
                return await response.Content.ReadAsByteArrayAsync().ConfigureAwait(false);
            }
        }

        private async Task<T> GetAsync<T>(string path)
        {
            using (var request = CreateRequest(HttpMethod.Get, path))
            using (var response = await http.SendAsync(request).ConfigureAwait(false))
            {
                await EnsureSuccess(response).ConfigureAwait(false);
                return json.Deserialize<T>(await response.Content.ReadAsStringAsync().ConfigureAwait(false));
            }
        }

        private async Task<T> PostAsync<T>(Uri server, string path, object body, bool authenticated)
        {
            using (var request = new HttpRequestMessage(HttpMethod.Post, new Uri(server, path)))
            {
                if (authenticated)
                    request.Headers.Authorization = new System.Net.Http.Headers.AuthenticationHeaderValue("Bearer", token);
                request.Content = new StringContent(json.Serialize(body), Encoding.UTF8, "application/json");
                using (var response = await http.SendAsync(request).ConfigureAwait(false))
                {
                    await EnsureSuccess(response).ConfigureAwait(false);
                    return json.Deserialize<T>(await response.Content.ReadAsStringAsync().ConfigureAwait(false));
                }
            }
        }

        private HttpRequestMessage CreateRequest(HttpMethod method, string path)
        {
            if (!IsLoggedIn)
                throw new InvalidOperationException("Сначала войдите на сервер класса");
            var request = new HttpRequestMessage(method, new Uri(baseUri, path));
            request.Headers.Authorization = new System.Net.Http.Headers.AuthenticationHeaderValue("Bearer", token);
            return request;
        }

        private static async Task EnsureSuccess(HttpResponseMessage response)
        {
            if (response.IsSuccessStatusCode)
                return;
            var body = await response.Content.ReadAsStringAsync().ConfigureAwait(false);
            if (response.StatusCode == HttpStatusCode.Unauthorized)
                throw new InvalidOperationException("Сеанс истёк. Войдите заново.");
            throw new InvalidOperationException("Сервер класса: " + response.StatusCode + " " + body);
        }

        public void Dispose() { http.Dispose(); }

        private sealed class LoginReply
        {
            public string token { get; set; }
            public string login { get; set; }
            public string role { get; set; }
        }

        private sealed class TestsReply
        {
            public List<ClassroomTest> tests { get; set; }
        }

        private sealed class SubmissionReply
        {
            public int submission_id { get; set; }
        }
    }

    internal sealed class ClassroomTest
    {
        public string input { get; set; }
        public string expected { get; set; }
    }

    internal sealed class SubmissionStatus
    {
        public string status { get; set; }
        public string detail { get; set; }
    }
}
