using System;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Web.Script.Serialization;
using VisualPascalABC.Classroom;

namespace VisualPascalABC
{
    public partial class Form1
    {
        private static readonly Regex ClassroomTaskHeader = new Regex(
            @"\A\uFEFF?\{abc\.pro-task:\s*([0-9a-fA-F-]{36})\s*\r?\n",
            RegexOptions.IgnoreCase | RegexOptions.Compiled);
        private readonly ClassroomClient classroomClient = new ClassroomClient();
        private ToolStripMenuItem classroomLoginItem;
        private ToolStripMenuItem classroomLogoutItem;
        private ToolStripMenuItem classroomDownloadItem;
        private ToolStripStatusLabel classroomStatus;
        private System.Windows.Forms.Timer classroomHeartbeat;
        private int latestClassroomSubmission;
        private int classroomRunGeneration;

        private string ClassroomSettingsPath
        {
            get
            {
                return Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
                    "abc.pro", "classroom.json");
            }
        }

        private void InitializeClassroom()
        {
            var menu = new ToolStripMenuItem("Класс");
            classroomLoginItem = new ToolStripMenuItem("Войти на сервер...");
            classroomLogoutItem = new ToolStripMenuItem("Выйти");
            classroomDownloadItem = new ToolStripMenuItem("Скачать задания...");
            classroomLoginItem.Click += async (sender, args) => await ClassroomLoginAsync();
            classroomLogoutItem.Click += async (sender, args) =>
            {
                classroomRunGeneration++;
                latestClassroomSubmission = 0;
                await classroomClient.LogoutAsync();
                classroomStatus.Text = "Класс: вход не выполнен";
                UpdateClassroomMenu();
            };
            classroomDownloadItem.Click += async (sender, args) => await DownloadClassroomAssignmentsAsync();
            menu.DropDownItems.Add(classroomLoginItem);
            menu.DropDownItems.Add(classroomLogoutItem);
            menu.DropDownItems.Add(new ToolStripSeparator());
            menu.DropDownItems.Add(classroomDownloadItem);
            menuStrip1.Items.Add(menu);
            classroomStatus = new ToolStripStatusLabel("Класс: вход не выполнен");
            statusStrip1.Items.Add(classroomStatus);
            UpdateClassroomMenu();
            classroomHeartbeat = new System.Windows.Forms.Timer { Interval = 30000 };
            classroomHeartbeat.Tick += async (sender, args) =>
            {
                if (!classroomClient.IsLoggedIn)
                    return;
                try { await classroomClient.HeartbeatAsync(); }
                catch (Exception error) { classroomStatus.Text = "Класс: нет связи — " + error.Message; }
            };
            classroomHeartbeat.Start();
        }

        private void UpdateClassroomMenu()
        {
            classroomLogoutItem.Enabled = classroomClient.IsLoggedIn;
            classroomDownloadItem.Enabled = classroomClient.IsLoggedIn;
        }

        private SavedClassroomSettings LoadClassroomSettings()
        {
            try
            {
                if (File.Exists(ClassroomSettingsPath))
                    return new JavaScriptSerializer().Deserialize<SavedClassroomSettings>(
                        File.ReadAllText(ClassroomSettingsPath));
            }
            catch (Exception) { }
            return new SavedClassroomSettings { port = 8080 };
        }

        private async Task ClassroomLoginAsync()
        {
            var saved = LoadClassroomSettings();
            using (var dialog = new ClassroomLoginDialog(saved.address, saved.port, saved.login,
                                                         saved.https, UserOptions.DarkTheme))
            {
                if (dialog.ShowDialog(this) != DialogResult.OK)
                    return;
                classroomRunGeneration++;
                latestClassroomSubmission = 0;
                classroomStatus.Text = "Класс: подключение...";
                try
                {
                    await classroomClient.LoginAsync(dialog.Address, dialog.Port, dialog.Https,
                                                     dialog.Login, dialog.Password);
                    var settings = new SavedClassroomSettings {
                        address = dialog.Address, port = dialog.Port,
                        login = dialog.Login, https = dialog.Https
                    };
                    Directory.CreateDirectory(Path.GetDirectoryName(ClassroomSettingsPath));
                    File.WriteAllText(ClassroomSettingsPath,
                        new JavaScriptSerializer().Serialize(settings), new UTF8Encoding(false));
                    classroomStatus.Text = "Класс: " + classroomClient.Login + " подключён";
                    UpdateClassroomMenu();
                }
                catch (Exception error)
                {
                    classroomStatus.Text = "Класс: вход не выполнен";
                    MessageBox.Show(this, error.Message, "Вход в класс", MessageBoxButtons.OK,
                                    MessageBoxIcon.Warning);
                }
            }
        }

        private async Task DownloadClassroomAssignmentsAsync()
        {
            try
            {
                var archive = await classroomClient.DownloadAssignmentsAsync();
                using (var save = new SaveFileDialog {
                    Filter = "ZIP-архив (*.zip)|*.zip", FileName = "abc-pro-assignments.zip",
                    Title = "Сохранить задания"
                })
                {
                    if (save.ShowDialog(this) != DialogResult.OK)
                        return;
                    File.WriteAllBytes(save.FileName, archive);
                    classroomStatus.Text = "Задания скачаны. Распакуйте архив и откройте файл .pas";
                }
            }
            catch (Exception error)
            {
                MessageBox.Show(this, error.Message, "Задания", MessageBoxButtons.OK,
                                MessageBoxIcon.Warning);
            }
        }

        internal async void ClassroomProgramStarted(string sourceFile, string source)
        {
            var match = ClassroomTaskHeader.Match(source ?? "");
            if (!match.Success)
                return;
            if (!classroomClient.IsLoggedIn)
            {
                classroomStatus.Text = "Задание: войдите на сервер класса для проверки";
                return;
            }
            var generation = ++classroomRunGeneration;
            var taskId = match.Groups[1].Value;
            try
            {
                await classroomClient.StartTaskAsync(taskId);
                classroomStatus.Text = "Задача запущена; выполняются открытые тесты...";
                var tests = await classroomClient.PublicTestsAsync(taskId);
                LocalTestResult local;
                try { local = await Task.Run(() => LocalTestRunner.Run(sourceFile, tests)); }
                catch (Exception error)
                {
                    local = new LocalTestResult { Total = tests.Count,
                        Message = "Локальная проверка не выполнена: " + error.Message };
                }
                if (generation != classroomRunGeneration || IsDisposed)
                    return;
                classroomStatus.Text = "Локально " + local.Passed + "/" + local.Total +
                                       "; сервер проверяет решение...";
                var submissionId = await classroomClient.SubmitAsync(taskId, source,
                                                                     local.Passed, local.Total);
                latestClassroomSubmission = submissionId;
                await PollClassroomSubmissionAsync(submissionId);
            }
            catch (Exception error)
            {
                if (generation == classroomRunGeneration && !IsDisposed)
                    classroomStatus.Text = "Класс: " + error.Message;
            }
        }

        private async Task PollClassroomSubmissionAsync(int submissionId)
        {
            for (var attempt = 0; attempt < 90; attempt++)
            {
                await Task.Delay(2000);
                if (submissionId != latestClassroomSubmission || IsDisposed)
                    return;
                var state = await classroomClient.SubmissionAsync(submissionId);
                if (state.status == "pending")
                    continue;
                classroomStatus.Text = state.status == "solved" ? "Задача решена" :
                    state.status == "wrong" ? "Задача решена неверно" :
                    "Ошибка серверной проверки: " + state.detail;
                return;
            }
            classroomStatus.Text = "Серверная проверка продолжается; статус виден преподавателю";
        }

        private sealed class SavedClassroomSettings
        {
            public string address { get; set; }
            public int port { get; set; }
            public string login { get; set; }
            public bool https { get; set; }
        }
    }
}
