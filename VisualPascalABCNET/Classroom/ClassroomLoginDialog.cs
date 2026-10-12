using System;
using System.Drawing;
using System.Windows.Forms;

namespace VisualPascalABC.Classroom
{
    internal sealed class ClassroomLoginDialog : Form
    {
        private readonly TextBox address = new TextBox();
        private readonly NumericUpDown port = new NumericUpDown();
        private readonly TextBox login = new TextBox();
        private readonly TextBox password = new TextBox();
        private readonly CheckBox https = new CheckBox();

        public string Address { get { return address.Text.Trim(); } }
        public int Port { get { return (int)port.Value; } }
        public string Login { get { return login.Text.Trim(); } }
        public string Password { get { return password.Text; } }
        public bool Https { get { return https.Checked; } }

        public ClassroomLoginDialog(string lastAddress, int lastPort, string lastLogin,
                                    bool useHttps, bool dark)
        {
            Text = "Вход в класс abc.pro";
            FormBorderStyle = FormBorderStyle.FixedDialog;
            StartPosition = FormStartPosition.CenterParent;
            MinimizeBox = false;
            MaximizeBox = false;
            ShowInTaskbar = false;
            ClientSize = new Size(400, 260);

            var layout = new TableLayoutPanel {
                Dock = DockStyle.Fill, Padding = new Padding(16), ColumnCount = 2, RowCount = 6
            };
            layout.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 130));
            layout.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
            for (var i = 0; i < 6; i++)
                layout.RowStyles.Add(new RowStyle(SizeType.Absolute, i == 5 ? 45 : 36));
            address.Text = lastAddress;
            port.Minimum = 1;
            port.Maximum = 65535;
            port.Value = lastPort > 0 ? lastPort : 8080;
            login.Text = lastLogin;
            password.UseSystemPasswordChar = true;
            https.Text = "HTTPS";
            https.Checked = useHttps;
            AddRow(layout, 0, "IP-адрес сервера", address);
            AddRow(layout, 1, "Порт", port);
            AddRow(layout, 2, "Логин", login);
            AddRow(layout, 3, "Пароль", password);
            layout.Controls.Add(https, 1, 4);

            var buttons = new FlowLayoutPanel { Dock = DockStyle.Fill, FlowDirection = FlowDirection.RightToLeft };
            var connect = new Button { Text = "Войти", DialogResult = DialogResult.OK, Width = 90 };
            var cancel = new Button { Text = "Отмена", DialogResult = DialogResult.Cancel, Width = 90 };
            buttons.Controls.Add(connect);
            buttons.Controls.Add(cancel);
            layout.Controls.Add(buttons, 0, 5);
            layout.SetColumnSpan(buttons, 2);
            Controls.Add(layout);
            AcceptButton = connect;
            CancelButton = cancel;
            if (dark)
                ApplyDark(layout);
        }

        private static void AddRow(TableLayoutPanel layout, int row, string caption, Control control)
        {
            var label = new Label { Text = caption, TextAlign = ContentAlignment.MiddleLeft, Dock = DockStyle.Fill };
            control.Dock = DockStyle.Fill;
            layout.Controls.Add(label, 0, row);
            layout.Controls.Add(control, 1, row);
        }

        private void ApplyDark(Control control)
        {
            BackColor = Color.FromArgb(37, 37, 38);
            ForeColor = Color.Gainsboro;
            control.BackColor = BackColor;
            control.ForeColor = ForeColor;
            foreach (Control child in control.Controls)
            {
                if (child is TextBox || child is NumericUpDown)
                {
                    child.BackColor = Color.FromArgb(30, 30, 30);
                    child.ForeColor = Color.Gainsboro;
                }
                else
                    ApplyDark(child);
            }
        }
    }
}
