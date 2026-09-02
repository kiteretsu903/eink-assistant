using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;
using System.Threading;
using System.Windows.Automation;

internal static class NightLightControl
{
    private const string TurnOnAutomationId =
        "SystemSettings_Display_BlueLight_ManualToggleOn_Button";
    private const string TurnOffAutomationId =
        "SystemSettings_Display_BlueLight_ManualToggleOff_Button";
    private const uint WmClose = 0x0010;
    private const uint WmSysCommand = 0x0112;
    private const int ScClose = 0xF060;
    private const uint SwpNoSize = 0x0001;
    private const uint SwpNoActivate = 0x0010;
    private const uint SwpNoOwnerZOrder = 0x0200;
    private const uint SwpAsyncWindowPos = 0x4000;

    private delegate bool EnumWindowsProc(IntPtr window, IntPtr parameter);

    [DllImport("user32.dll")]
    private static extern bool EnumWindows(EnumWindowsProc callback, IntPtr parameter);

    [DllImport("user32.dll")]
    private static extern bool EnumChildWindows(IntPtr parent, EnumWindowsProc callback, IntPtr parameter);

    [DllImport("user32.dll")]
    private static extern uint GetWindowThreadProcessId(IntPtr window, out uint processId);

    [DllImport("user32.dll")]
    private static extern bool IsWindowVisible(IntPtr window);

    [DllImport("user32.dll")]
    private static extern bool PostMessage(IntPtr window, uint message, IntPtr wParam, IntPtr lParam);

    [DllImport("user32.dll")]
    private static extern bool SetWindowPos(IntPtr window, IntPtr insertAfter, int x, int y,
        int width, int height, uint flags);

    private static int Main(string[] arguments)
    {
        if (arguments.Length == 0) return 64;
        bool query = string.Equals(arguments[0], "query", StringComparison.OrdinalIgnoreCase);
        bool targetEnabled = string.Equals(arguments[0], "on", StringComparison.OrdinalIgnoreCase);
        if (!query && !targetEnabled &&
            !string.Equals(arguments[0], "off", StringComparison.OrdinalIgnoreCase)) return 64;

        bool settingsWasOpen = FindSettingsWindow() != IntPtr.Zero;
        bool quiet = arguments.Any(value => value == "--quiet");
        bool hideSettings = quiet && (!settingsWasOpen || arguments.Any(value => value == "--owned-settings"));
        AutomationElement action;
        bool? nativeEnabled = OpenNightLightSettingsAndReadState(hideSettings, out action);
        if (!nativeEnabled.HasValue) return 65;

        try
        {
            bool enabled = nativeEnabled.Value;
            if (query)
            {
                Console.WriteLine("state={0};opened={1}", enabled ? "on" : "off", settingsWasOpen ? "0" : "1");
                return (enabled ? 12 : 10) + (settingsWasOpen ? 0 : 1);
            }

            if (enabled != targetEnabled)
            {
                object rawPattern;
                if (!action.TryGetCurrentPattern(InvokePattern.Pattern, out rawPattern)) return 66;
                ((InvokePattern)rawPattern).Invoke();
            }
            for (int attempt = 0; attempt < 100; ++attempt)
            {
                Thread.Sleep(50);
                AutomationElement nextAction;
                bool? nextState = ReadNativeState(hideSettings, out nextAction);
                if (nextState.HasValue && nextState.Value == targetEnabled)
                {
                    Console.WriteLine("state={0}", targetEnabled ? "on" : "off");
                    bool ownedSettings = !settingsWasOpen || arguments.Any(value => value == "--owned-settings");
                    if (ownedSettings || arguments.Any(value => value == "--close")) CloseSettingsWindow(ownedSettings);
                    return 0;
                }
            }
        }
        catch (ElementNotAvailableException)
        {
            return 67;
        }

        return 68;
    }

    private static bool? OpenNightLightSettingsAndReadState(bool hideSettings, out AutomationElement action)
    {
        action = null;
        try
        {
            Process.Start(new ProcessStartInfo("ms-settings:nightlight") { UseShellExecute = true });
        }
        catch
        {
            return null;
        }

        for (int attempt = 0; attempt < 160; ++attempt)
        {
            bool? state = ReadNativeState(hideSettings, out action);
            if (state.HasValue) return state;
            Thread.Sleep(50);
        }
        return null;
    }

    private static bool? ReadNativeState(bool hideSettings, out AutomationElement action)
    {
        action = null;
        try
        {
            IntPtr window = FindSettingsWindow(hideSettings);
            if (window == IntPtr.Zero) return null;
            if (hideSettings) MoveSettingsOffscreen(window);
            AutomationElement root = AutomationElement.FromHandle(window);
            Condition turnOff = new PropertyCondition(AutomationElement.AutomationIdProperty, TurnOffAutomationId);
            Condition turnOn = new PropertyCondition(AutomationElement.AutomationIdProperty, TurnOnAutomationId);
            action = root.FindFirst(TreeScope.Descendants, new OrCondition(turnOff, turnOn));
            if (action != null && action.Current.IsEnabled)
                return string.Equals(action.Current.AutomationId, TurnOffAutomationId, StringComparison.Ordinal);
        }
        catch (ElementNotAvailableException)
        {
        }
        action = null;
        return null;
    }

    private static HashSet<uint> SystemSettingsProcessIds()
    {
        return new HashSet<uint>(Process.GetProcessesByName("SystemSettings").Select(process => (uint)process.Id));
    }

    private static void MoveSettingsOffscreen(IntPtr window)
    {
        SetWindowPos(window, new IntPtr(1), -32000, -32000, 0, 0,
            SwpNoSize | SwpNoActivate | SwpNoOwnerZOrder | SwpAsyncWindowPos);
    }

    private static IntPtr FindSettingsWindow(bool includeHidden = false)
    {
        HashSet<uint> settingsIds = SystemSettingsProcessIds();
        IntPtr result = IntPtr.Zero;
        EnumWindows(delegate(IntPtr window, IntPtr parameter)
        {
            if (!includeHidden && !IsWindowVisible(window)) return true;
            uint topLevelProcessId;
            GetWindowThreadProcessId(window, out topLevelProcessId);
            if (settingsIds.Contains(topLevelProcessId))
            {
                result = window;
                return false;
            }
            bool ownsSettingsChild = false;
            EnumChildWindows(window, delegate(IntPtr child, IntPtr childParameter)
            {
                uint processId;
                GetWindowThreadProcessId(child, out processId);
                if (settingsIds.Contains(processId)) ownsSettingsChild = true;
                return !ownsSettingsChild;
            }, IntPtr.Zero);
            if (!ownsSettingsChild) return true;
            result = window;
            return false;
        }, IntPtr.Zero);
        return result;
    }

    private static void CloseSettingsWindow(bool terminateIfNecessary)
    {
        IntPtr window = FindSettingsWindow(true);
        if (window == IntPtr.Zero) return;
        try
        {
            AutomationElement element = AutomationElement.FromHandle(window);
            object rawPattern;
            if (element.TryGetCurrentPattern(WindowPattern.Pattern, out rawPattern))
                ((WindowPattern)rawPattern).Close();
        }
        catch (ElementNotAvailableException)
        {
        }
        PostMessage(window, WmSysCommand, new IntPtr(ScClose), IntPtr.Zero);
        PostMessage(window, WmClose, IntPtr.Zero, IntPtr.Zero);
        for (int attempt = 0; attempt < 20 && FindSettingsWindow(false) != IntPtr.Zero; ++attempt)
            Thread.Sleep(25);
        if (terminateIfNecessary && FindSettingsWindow(false) != IntPtr.Zero)
        {
            foreach (Process process in Process.GetProcessesByName("SystemSettings"))
            {
                try { process.Kill(); }
                catch { }
            }
            for (int attempt = 0; attempt < 20 && FindSettingsWindow(false) != IntPtr.Zero; ++attempt)
                Thread.Sleep(25);
        }
    }
}
