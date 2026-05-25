/*

><>$ echo HAVE A HAVE A LOOK ONE POUND FIS

tysm for using my open source fiscmd console app :D

for context, ts screams for .net 6.0 long term support, which requires windows 7 or newer
cuz well... obviously this console app was too uh simple :skull:
and also... cross platform :D (works well + tested on windows + real linux)

its open source so u can edit/add anything here and give it a name as long as ur not copying me :skull:
especially editing the PrintPrompt() function, cuz thats where the ><>$ thing comes from

made entirely using the default csharp language

bye :D

more than 5 thousand worth of lines :sob::pray: (honorable mention: all made by chatgpt and a 13 yo teenager named swindow)

*/

using Microsoft.VisualBasic;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
// parse quotes helper (to let file interaction functions handles spacing in files' name)
using System.Text.RegularExpressions;
using System.Net.NetworkInformation;

namespace fis
{
    class Program
    {
        // static strings
        static bool showDir = true;
        // current directory
        static string currentDir = Directory.GetCurrentDirectory();
        // custom directory
        static string sys32fol = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.Windows), "System32");
        // history (for autocomplete)
        static List<string> history = new List<string>();
        static int historyIndex = -1;

        // importable commands
        static bool importedFissnake = false;
        static bool importedFisscript = false;
        static bool importedFisdraw = false;
        static bool importedStars = false;
		// if u wanna add more importable commands, just add this:
		// static bool importedCommand = false;
		// note that it MUST be false here and true later
		// u can use it in the ImportCmd() function (or void whatever u say)

        // current fore/background
        static ConsoleColor currentFg = ConsoleColor.Gray;
        static ConsoleColor currentBg = ConsoleColor.Black;

        // commands for tab autocomplete (well... NOT exactly real ><>$ autocom)
        static List<string> commands = new List<string>
        {
            "exit","info","inf","i","aboutthiscmd",
            "aboutthisconsole","abtthiscmd","abtthisconsole",
            "help","helpmepls","/?","?",
            "rng","calc","simplecalc",
            "example","ex","eg",
            "cls","color",
            "time","fulltime","when","watsthetime",
            "beep","watchtime","crash","requirement",
            "req","rq","require","required","currentos",
            "curos","os","cd","mkdir","rm","rmdir",
            "toggleShowDir","toggleshowdir","touch","ls",
            "dir","cat","copy","cop","cope","move","mov","mv",
            "run","open","launch","history","hist","his","clearhistory",
            "clearhist","clearhis","clshistory","clshist","clshis",
            "showversion","version","showver","ver","settitle","title",
            "printfish","printfis","prntfish","prntfis","prnfis",
            "prnfish","flipcoin","flipacoin","coin","morecoins", "headandtail",
            "headsandtails","headsntails","headntail","tree",
            "systeminfo","systeminf","systemi","sysinfo","sysinf","sysi",
            "memoryinfo","memoryinf","memoryi","meminfo","meminf",
            "memi","benchmark","benchm","bmark",
            "zip","zipfolder","zipfol",
            "unzip","unzipfile","unzipfolder","unzipfol",
            "rename","ren","rn (ye, no not rM)",
            "hash","whoami","showupdates","update","showupdate","updates",
            "showlog","showlogs","logs","log","echo","import",
            "top","fistop","taskmgr",
            "killproc","fiskill","kill",
            "sudo",
            "fissnake","snake",
            "fisscript","script","scriptfile","scr",
            "fisdraw","draw","fispaint",
            "netwatch",
            "diskparty","diskusages",
            "fistars","fisstar","stars","star"
        };

        static void Main(string[] args)
        {
            Console.CursorVisible = false;

            Console.Title = "fiscmd ><>";

            Console.ForegroundColor = ConsoleColor.Cyan;
            PrintFisCoolAsf(true, 25, false);
            TypeWrite(" fiscmd initialized, welcome :D", 25, false);
            Console.WriteLine();

            // ResForegroundColor();
            Console.ResetColor(); // theres barely any color set when first ran

            Console.CursorVisible = true;

            while (true)
            {
                // apply saved colors
                Console.ForegroundColor = currentFg;
                Console.BackgroundColor = currentBg;

                // prompt
                Console.ForegroundColor = ConsoleColor.Green;
                //              Console.Write("><>$ ");
                PrintPrompt();


                // unused prompt cd logic (obsollete by the PrintPrompt() function)
                /*
                string normalized = Path.GetFullPath(currentDir).TrimEnd('\\');

                if (Directory.Exists(normalized))
                {
                    if (normalized.Equals("C:", StringComparison.OrdinalIgnoreCase))
                    {
                        Console.Write("><[main os partition]>$ ");
                    }
                    else if (normalized.EndsWith("Windows\\System32", StringComparison.OrdinalIgnoreCase))
                    {
                        Console.Write("><[important admin folder]>$ ");
                    }
                    else if (normalized.EndsWith("win-x64", StringComparison.OrdinalIgnoreCase))
                    {
                        Console.Write("><>$ ");
                    }
                    else
                    {
                        Console.Write($"><[{normalized}]>$ ");
                    }
                }
                else
                {
                    Console.Write("><>$ ");
                }
                */

                Console.ForegroundColor = currentFg;

                string input = ReadCommand();
                if (string.IsNullOrWhiteSpace(input)) continue;

                history.Add(input);
                historyIndex = history.Count;

//              input = input.Trim().ToLower();

                string rawInput = input.Trim();
                string lowerInput = rawInput.ToLower();

                // old ver:
                //string[] parts = input.Split(' ', StringSplitOptions.RemoveEmptyEntries);
                //string[] parts = lowerInput.Split(' ', StringSplitOptions.RemoveEmptyEntries);

                // new ver:
                string[] parts = ParseQuotedArgs(rawInput);
                string command = parts[0].ToLower();

                switch (command)
                {
                    case "exit":
                        Console.ForegroundColor = ConsoleColor.Cyan;
                        Console.WriteLine("tysm for using our console app :D");
                        Console.ForegroundColor = ConsoleColor.White;
                        Console.WriteLine("any key to exit sir ;-;");
                        Console.ReadKey(true);
                        Console.ResetColor(); // yk what i mean (who uses ResForegroundColor() upon exiting :skull:)
                        return;

                    case "info":
                    case "inf":
                    case "i":
                    case "about":
                    case "abt":
                    case "aboutthiscmd":
                    case "aboutthisconsole":
                    case "abtthisconsole":
                    case "abtthiscmd": ShowInfo(); break;
                    
                    case "help":
                    case "helpmepls":
                    case "/?":
                    case "?": ShowHelp(); break;

                    case "rng": RunRng(); break;

                    case "calc":
                    case "simplecalc": RunCalc(); break;

                    case "example":
                    case "ex":
                    case "eg": ShowExample(); break;

                    case "clear":
                    case "cls": Console.Clear(); break;

                    case "time":
                    case "when":
                    case "watsthetime": ShowTime(); break;

                    case "fulltime": ShowFullTime(); break;

                    case "beep": RunBeep(parts); break;

                    case "watchtime": WatchTime(); break;

                    case "crash": FakeCrash(); break;

                    case "requirement":
                    case "req":
                    case "rq":
                    case "require":
                    case "required":
                        Console.ForegroundColor = ConsoleColor.DarkMagenta;
                        Console.WriteLine(".NET 6.0 (long-term support)");
                        ResForegroundColor();
                        break;

                    case "currentos":
                    case "curos":
                    case "os": DetectOS(); break;

                    // file modifying functions
                    case "copy":
                    case "cop":
                    case "cope": Copy(parts); break;

                    case "cp": 
                        Console.ForegroundColor = ConsoleColor.Red;
                        Console.WriteLine("what command are u tryna enter :pray::wilted-rose:");
                        ResForegroundColor();
                        break;

                    case "mkdir":
                        Mkdir(parts);
                        break;

                    case "rmdir":
                        Rmdir(parts);
                        break;

                    case "rm":
                        Rm(parts);
                        break;

                    case "cd":
                        Cd(parts);
                        break;

                    case "toggleShowDir":
                    case "toggleshowdir": toggleShowDir(); break;

                    case "touch":
                        Touch(parts);
                        break;

                    case "ls":
                    case "dir":
                        ls(parts);
                        break;

                    case "cat":
                        Cat(parts);
                        break;

                    case "move":
                    case "mov":
                    case "mv": Move(parts); break;

                    case "run":
                    case "open":
                    case "launch": RunCommand(parts); break;


                    // goofy miscs
                    case "history":
                    case "his":
                    case "hist": ShowHistory(); break;

                    case "clearhistory":
                    case "clshistory":
                    case "clearhis":
                    case "clshis":
                    case "clearhist":
                    case "clshist": ClearHistory(); break;

                    case "showversion":
                    case "showver":
                    case "version":
                    case "ver":
                    case "v": ShowVersion(); break;

                    case "settitle":
                    case "title": SetTitle(parts); break;

                    case "printfis":
                    case "printfish":
                    case "prnfish":
                    case "prnfis":
                    case "prntfish":
                    case "prntfis": PrintFis(); break;

                    case "flipcoin":
                    case "flipacoin":
                    case "coin":
                    case "morecoins":
                    case "morecoin":
                    case "headntail":
                    case "headsntails":
                    case "headandtail":
                    case "headsandtails": FlipCoin(); break;

                    case "tree": TreeCommand(parts); break;

                    case "systeminfo":
                    case "systeminf":
                    case "systemi":
                    case "sysinfo":
                    case "sysinf":
                    case "sysi": SysInfo(); break;

                    case "memoryinfo":
                    case "memoryinf":
                    case "memoryi":
                    case "meminfo":
                    case "meminf":
                    case "memi": MemInfo(); break;

                    case "benchmark":
                    case "bmark":
                    case "benchm": Benchmark(); break;

                    case "zip":
                    case "zipfolder":
                    case "zipfol": ZipFolder(parts); break;

                    case "unzip":
                    case "unzipfile":
                    case "unzipfolder":
                    case "unzipfol": UnzipFile(parts); break;

                    case "rename":
                    case "ren":
                    case "rn": Rename(parts); break;

                    case "hash":
                    case "sha256": HashFile(parts); break;

                    case "whoami":
                    case "%userprofile%": ShowUser(); break;

                    case "echo": DoEcho(rawInput); break;

                    case "showupdates":
                    case "showupdate":
                    case "showlogs":
                    case "showlog":
                    case "updates":
                    case "update":
                    case "logs":
                    case "log": ShowUpdate(); break;

                    case "sudo": Console.WriteLine("nice try :skull:"); break; // sudo joke

                    case "initializefis":
                    case "initfis":
                        PrintFisCoolAsf(true, 25, false);
                        TypeWrite(" fiscmd initialized, welcome :D", 25, false);
                        ResForegroundColor();
                        Console.WriteLine();
                        break;

                    case "netwatch": NetWatch(); break;

                    case "diskparty": DiskParty(); break;

                    // importable command
                    case "importcmd":
                    case "import":
                        ImportCmd(parts);
                        break;

                    case "fissnake":
                    case "snake":
                        bool ye2 = WarnNotImported(importedFissnake);
                        if (ye2) break;

                        Snake(parts);
                        break;

                    case "fisscript":
                    case "scriptfile":
                    case "script":
                    case "scr":
                        bool ye3 = WarnNotImported(importedFisscript);
                        if (ye3) break;

                        FisScript(parts);
                        break;

                    case "fisdraw":
                    case "draw":
                    case "fispaint": // joke name grabbed from "mspaint"
                        bool ye4 = WarnNotImported(importedFisdraw);
                        if (ye4) break;

                        FisDraw(parts);
                        break;

                    case "fisstar":
                    case "fisstars":
                    case "star":
                    case "stars":
                        bool ye6 = WarnNotImported(importedStars);
                        if (ye6) break;

                        Stars();
                        break;

                    // unimportable command that used to be importable back then

                    case "top":
                    case "fistop":
                    case "taskmgr": Taskmgr(parts); break;

                    case "kill":
                    case "fiskill":
                    case "killproc": KillProc(parts); break;

                    default:
                        if (parts[0] == "color") SetColor(input);
                        else
                        {
                            Console.ForegroundColor = ConsoleColor.Red;
                            Console.WriteLine("ts is barely even a command");
                            ResForegroundColor();
                        }
                        break;
                }
            }
        }

        // let the file interaction voids SUPPORT SPACES IN FILES' NAMES NOW
        // (its my time to SHINE...)
        //
        // Main() already uses this for command parsing,
        // so DO NOT manually call this again in every void/function.
        //
        // for single-path file interaction voids, use:
        // string [any] = Path.Combine(currentDir, string.Join(" ", args.Skip(1)));
        //
        // for multi-path commands (copy/move/rename/etc),
        // REQUIRE QUOTES in paths with spaces.
        static string[] ParseQuotedArgs(string input)
        {
            var matches = Regex.Matches(input, "\"([^\"]*)\"|(\\S+)");

            List<string> result = new List<string>();

            foreach (Match match in matches)
            {
                if (match.Groups[1].Success)
                    result.Add(match.Groups[1].Value);
                else
                    result.Add(match.Groups[2].Value);
            }

            return result.ToArray();
        }

        // also helper void for Fisscript() (for the print command)
        static string ParseVars(string text, Dictionary<string, string> vars)
        {
            foreach (var v in vars)
            {
                text = text.Replace($"{{{v.Key}}}", v.Value);
            }

            return text;
        }

        // useless typewriting function only used in ShowUpdate() command :sob::pray:
        static void TypeWrite(string input, int delay=10, bool newline=true)
        {
            foreach (char c in input)
            {
                Console.Write(c);
                Thread.Sleep(delay);
            }
            if (newline)
            {
                Console.WriteLine();
            }
        }

        // "useless" print the fish ascii
        static void PrintFisCoolAsf(bool type, int delay = 10, bool newline = true)
        {
            if (type)
            {
                Console.ForegroundColor = ConsoleColor.DarkBlue;
                Console.Write(">");
                Thread.Sleep(delay);
                Console.ForegroundColor = ConsoleColor.DarkCyan;
                Console.Write("<");
                Thread.Sleep(delay);
                Console.ForegroundColor = ConsoleColor.Cyan;
                Console.Write(">");
                Thread.Sleep(delay);

                if (newline)
                {
                    Console.WriteLine();
                }
            }
            else
            {
                Console.ForegroundColor = ConsoleColor.Cyan;
                Console.Write(">");
                Console.ForegroundColor = ConsoleColor.DarkCyan;
                Console.Write("<");
                Console.ForegroundColor = ConsoleColor.Cyan;
                Console.Write(">");
            }
        }

        static void ShowUser()
        {
            Console.ForegroundColor = ConsoleColor.Cyan;
            Console.WriteLine($"ur {Environment.UserName}");
            Console.WriteLine("u can still see ur name by using the command \"sysinfo\"");

            ResForegroundColor();
        }

        static void ShowInfo()
        {
            Console.ForegroundColor = ConsoleColor.Cyan;

            Console.WriteLine("tysm for using our console app :D");
            Console.WriteLine();
            Console.ForegroundColor = ConsoleColor.Yellow;
            Console.WriteLine("welcome to a console app i call \"fiscmd\" :D");
            Console.ForegroundColor = ConsoleColor.Cyan;
            Console.WriteLine("in this very simple console app i made...");
            Console.WriteLine("u can do uh...");
            Console.ForegroundColor = ConsoleColor.Yellow;
            Console.WriteLine("pretty basic stuff ;-;");
            Console.WriteLine();
            Console.ResetColor();
            Console.WriteLine("basically thats it");
            Console.WriteLine(":ishowspeed-my-mom-is-kinda-homeless:");
            Console.WriteLine();
            Console.ForegroundColor = ConsoleColor.Red;
            Console.WriteLine("also forgr to mention ts screams for .NET 6.0 long-term support");
            Console.WriteLine("ye no if u do ctrl + c this would auto exits");
            Console.WriteLine("(IF u have ctrl + shift + c copy method enabled in windows or ur on linux)");
            Console.WriteLine();
            Console.ForegroundColor = ConsoleColor.Cyan;
            Console.WriteLine("see ya! :D");

            ResForegroundColor();

            // basically thats it
        }

        static void RunBeep(string[] parts)
        {
            int freq = 1000;
            int dur = 200;

            bool hasFreq = false;
            bool hasDur = false;
            bool useDefault = false;

            try
            {
                // no args -> show usage
                if (parts.Length == 1)
                {
                    Console.ForegroundColor = ConsoleColor.Cyan;
                    Console.WriteLine("usage: beep [/freq hz] [/dur ms] (/default)");
                    Console.WriteLine("(/default) - optional switch, will plays a 1000 hz frequency for 200 millesec");
                    Console.WriteLine("[/freq hz] - targetted frequency ranged from 37 to 32767 hz");
                    Console.WriteLine("[/dur ms] - duration of the beep (counts in ms)");
                    Console.ForegroundColor = ConsoleColor.Red;
                    Console.WriteLine("\nalso forgr to mention that these switches are basically useless on linux :sob:");
                    ResForegroundColor();
                    return;
                }

                for (int i = 1; i < parts.Length; i++)
                {
                    string arg = parts[i].ToLower();

                    switch (arg)
                    {
                        case "/freq":
                            if (i + 1 < parts.Length && int.TryParse(parts[i + 1], out int f))
                            {
                                freq = f;
                                hasFreq = true;
                                i++;
                            }
                            else
                            {
                                Console.ForegroundColor = ConsoleColor.Cyan;
                                Console.WriteLine("usage: beep [/freq hz]");
                                Console.WriteLine("[/freq hz] - targetted frequency ranged from 37 to 32767 hz");
                                ResForegroundColor();
                                return;
                            }
                            break;

                        case "/dur":
                            if (i + 1 < parts.Length && int.TryParse(parts[i + 1], out int d))
                            {
                                dur = d;
                                hasDur = true;
                                i++;
                            }
                            else
                            {
                                Console.ForegroundColor = ConsoleColor.Cyan;
                                Console.WriteLine("usage: beep [/freq hz] [/dur ms] (/default)");
                                Console.WriteLine("[/dur ms] - duration of the beep (counts in ms)");
                                ResForegroundColor();
                                return;
                            }
                            break;

                        case "/default":
                            useDefault = true;
                            break;

                        default:
                            Console.ForegroundColor = ConsoleColor.Red;
                            Console.WriteLine("fym by that vro :sob::pray:");
                            ResForegroundColor();
                            return;
                    }
                }

                // illegal combos
                if (useDefault && (hasFreq || hasDur))
                {
                    Console.ForegroundColor = ConsoleColor.Red;
                    Console.WriteLine("fym by that vro :sob::pray:");
                    ResForegroundColor();
                    return;
                }

                // default mode
                if (useDefault)
                {
                    freq = 1000;
                    dur = 200;
                }

                // range check
                if (freq < 37 || freq > 32767)
                {
                    Console.ForegroundColor = ConsoleColor.Red;
                    Console.WriteLine("freq outta range bro (37 - 32767)");
                    ResForegroundColor();
                    return;
                }

                if (dur <= 0)
                {
                    Console.ForegroundColor = ConsoleColor.Red;
                    Console.WriteLine("duration gotta be > 0 :sob::pray:");
                    ResForegroundColor();
                    return;
                }

                // actual beep
                try
                {
                    Console.Beep(freq, dur);
                }
                catch
                {
                    Console.ForegroundColor = ConsoleColor.Cyan;
                    Console.WriteLine("detected a linux user :0");
                    Console.WriteLine("attempting to beep in a different way...");
                    Console.Write("\a");
                }
            }
            finally
            {
                ResForegroundColor();
            }
        }

        static void DetectOS()
        {
            bool isWindows = OperatingSystem.IsWindows();
            bool isLinux = OperatingSystem.IsLinux();
            
            if (isWindows)
            {
                Console.ForegroundColor = ConsoleColor.Cyan;
                Console.WriteLine("ur currently on windows");
            }
            else if (isLinux)
            {
                Console.ForegroundColor = ConsoleColor.Cyan;
                Console.WriteLine("ur currently on linux");
            }
            else
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("what os even is this bro");
            }

            ResForegroundColor();
        }

        static void WatchTime()
        {
            Console.WriteLine("press any key to stop...\n");

            while (!Console.KeyAvailable)
            {
                Console.SetCursorPosition(0, Console.CursorTop);
                Console.Write(DateTime.Now.ToString("HH:mm:ss"));
                Thread.Sleep(1000);
            }

            Console.ReadKey(true); // clear key
            Console.WriteLine();

            ResForegroundColor();
        }

        static void FakeCrash()
        {
            Console.Clear();
            Console.BackgroundColor = ConsoleColor.DarkBlue;
            Console.ForegroundColor = ConsoleColor.White;
            Console.Clear();

            Console.WriteLine("A problem has been detected and Windows has been shut down to prevent damage");
            Console.WriteLine("to your computer.");
            Console.WriteLine();
            Console.WriteLine("IRQL_NOT_LESS_OR_EQUAL");
            Console.WriteLine();
            Console.WriteLine("If this is the first time you've seen this Stop error screen,");
            Console.WriteLine("restart your computer. If this screen appears again, follow");
            Console.WriteLine("these steps:");
            Console.WriteLine();
            Console.WriteLine("Check to make sure any new hardware or software is properly installed.");
            Console.WriteLine("If this is a new installation, ask your hardware or software manufacturer");
            Console.WriteLine("for any Windows updates you might need.");
            Console.WriteLine();
            Console.WriteLine("If problems continue, disable or remove any newly installed hardware");
            Console.WriteLine("or software. Disable BIOS memory options such as caching or shadowing.");
            Console.WriteLine("If you need to use Safe Mode to remove or disable components, restart");
            Console.WriteLine("your computer, press F8 to select Advanced Startup Options, and then");
            Console.WriteLine("select Safe Mode.");
            Console.WriteLine();
            Console.WriteLine("Technical Information:");
            Console.WriteLine();
            Console.WriteLine("*** STOP: 0x0000000A (0xXXXXXXXX, 0xXXXXXXXX, 0xXXXXXXXX, 0xXXXXXXXX)");

            Console.CursorVisible = false;

            Console.ReadKey(true);
            Console.CursorVisible = true;
            ResForegroundColor();
            Console.BackgroundColor = currentBg;
            Console.Clear();
        }

        static string ReadCommand()
        {
            List<char> buffer = new();
            int cursor = 0;

            while (true)
            {
                // received key
                ConsoleKeyInfo key = Console.ReadKey(true);

                // enter
                if (key.Key == ConsoleKey.Enter)
                {
                    Console.WriteLine();
                    return new string(buffer.ToArray());
                }

                // arrow keys
                else if (key.Key == ConsoleKey.LeftArrow)
                {
                    if (cursor > 0)
                    {
                        cursor--;
                        Console.CursorLeft--;
                    }
                }

                else if (key.Key == ConsoleKey.RightArrow)
                {
                    if (cursor < buffer.Count)
                    {
                        cursor++;
                        Console.CursorLeft++;
                    }
                }

                // backscape (to delete a character)
                else if (key.Key == ConsoleKey.Backspace)
                {
                    if (cursor > 0)
                    {
                        buffer.RemoveAt(cursor - 1);
                        cursor--;

                        RedrawLine(buffer, cursor);
                    }
                }

                // same thing but with [DELETE] key
                else if (key.Key == ConsoleKey.Delete)
                {
                    if (cursor < buffer.Count)
                    {
                        buffer.RemoveAt(cursor);

                        RedrawLine(buffer, cursor);
                    }
                }

                // home and end keys ([FN] + [LEFT/RIGHT ARROW] on some devices' keyboard especially laptops if yk what i mean)
                else if (key.Key == ConsoleKey.Home)
                {
                    cursor = 0;
                    RedrawLine(buffer, cursor);
                }

                else if (key.Key == ConsoleKey.End)
                {
                    cursor = buffer.Count;
                    RedrawLine(buffer, cursor);
                }

                // show last command upon pressing the up/down arrow key
                else if (key.Key == ConsoleKey.UpArrow)
                {
                    if (history.Count > 0 && historyIndex > 0)
                    {
                        historyIndex--;

                        buffer = history[historyIndex].ToList();
                        cursor = buffer.Count;

                        RedrawLine(buffer, cursor);
                    }
                }

                else if (key.Key == ConsoleKey.DownArrow)
                {
                    if (historyIndex < history.Count - 1)
                    {
                        historyIndex++;

                        buffer = history[historyIndex].ToList();
                    }
                    else
                    {
                        historyIndex = history.Count;
                        buffer = new List<char>();
                    }

                    cursor = buffer.Count;

                    RedrawLine(buffer, cursor);
                }

                // tab autocorrect
                else if (key.Key == ConsoleKey.Tab)
                {
                    AutoComplete(ref buffer, ref cursor);
                }

                // [CTRL] + [L] cls logic
                else if (key.Modifiers.HasFlag(ConsoleModifiers.Control) && key.Key == ConsoleKey.L)
                {
                    Console.Clear();

                    Console.SetCursorPosition(0, 0);

                    RedrawLine(buffer, cursor);
                }

                // idk what is ts
                else if (!char.IsControl(key.KeyChar))
                {
                    buffer.Insert(cursor, key.KeyChar);
                    cursor++;

                    RedrawLine(buffer, cursor);
                }
            }
        }

        // RedrawLine helper void for ReadCommand void
        static void RedrawLine(List<char> buffer, int cursor)
        {
            int top = Console.CursorTop;

            string prompt = GetPrompt();

            Console.SetCursorPosition(0, top);

            // prompt color
            Console.ForegroundColor = ConsoleColor.Green;
            Console.Write(prompt);

            // input color
            Console.ForegroundColor = currentFg;
            int width = Console.WindowWidth;

            Console.Write(new string(' ', width - 1));

            Console.SetCursorPosition(0, top);

            Console.ForegroundColor = ConsoleColor.Green;
            Console.Write(prompt);

            Console.ForegroundColor = currentFg;
            Console.Write(new string(buffer.ToArray()));

            Console.SetCursorPosition(prompt.Length + cursor, top);
        }

        // GetPrompt helper void for RedrawLine void
        static string GetPrompt()
        {
            return $"><[{currentDir}]>$ ";
        }

        static void AutoComplete(ref List<char> buffer, ref int cursor)
        {
            string current = new string(buffer.ToArray()).ToLower();

            var matches = commands
                .Where(c => c.StartsWith(current))
                .ToList();

            if (matches.Count == 1)
            {
                buffer = matches[0].ToList();
                cursor = buffer.Count;

                RedrawLine(buffer, cursor);
            }
            else if (matches.Count > 1)
            {
                Console.WriteLine();

                Console.ForegroundColor = ConsoleColor.DarkGray;
                Console.WriteLine(string.Join("  ", matches));

                RedrawLine(buffer, cursor);
            }
        }

        static void SetColor(string input)
        {
            string[] parts = input.Split(' ', StringSplitOptions.RemoveEmptyEntries);

            if (parts.Length == 1)
            {
                Console.ForegroundColor = ConsoleColor.Cyan;
                Console.WriteLine("usage: color <fg> [bg] or color default");
                Console.WriteLine();
                Console.WriteLine("<fg> is foreground");
                Console.WriteLine("[bg] is background");
                Console.WriteLine();
                Console.WriteLine("both <fg> and [bg] can be between 0 and 15");

                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine();
                Console.WriteLine("use it wisely, u do NOT wanna be blinded by the texts and");
                Console.WriteLine("the foreground color being the same");

                Console.ForegroundColor = ConsoleColor.Yellow;
                Console.WriteLine();
                Console.WriteLine("also i recommend entering \"cls\" after setting the color");

                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("(because background color may NOT apply the whole thing without cls)");
                Console.WriteLine("(also foreground color wouldnt be applying to everything, just the prompt u type bru)");

                return;
            }

            if (parts.Length > 1 && parts[1].ToLower() == "default")
            {
                currentFg = ConsoleColor.Gray;
                currentBg = ConsoleColor.Black;
                ResForegroundColor();
                return;
            }

            // validate fg first (no committing yet)
            if (!int.TryParse(parts[1], out int fg) || fg < 0 || fg > 15)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("invalid color :c");
                return;
            }

            ConsoleColor newFg = (ConsoleColor)fg;
            ConsoleColor newBg = currentBg;

            // optional bg
            if (parts.Length > 2)
            {
                if (!int.TryParse(parts[2], out int bg) || bg < 0 || bg > 15)
                {
                    Console.ForegroundColor = ConsoleColor.Red;
                    Console.WriteLine("invalid background color :c");
                    return;
                }

                newBg = (ConsoleColor)bg;
            }

            // same color detection BEFORE applying anything
            if (newFg == newBg)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("dawg u set both foreground and background same :skull:");
                Console.WriteLine("still setting color?? (y/n)");

                if (showDir)
                {
                    Console.ForegroundColor = ConsoleColor.Cyan;
                    Console.Write("><[answer]> ");
                }
                else
                {
                    ResForegroundColor();
                }

                string confirm = Console.ReadLine()?.ToLower() ?? "n";

                if (confirm != "y")
                {
                    Console.WriteLine("uh ok good :sob::thumbs-up:");
                    return;
                }

                Console.WriteLine("good luck");
            }

            // commit changes AFTER checks
            currentFg = newFg;
            currentBg = newBg;

            ResForegroundColor();
        }

        static void ResForegroundColor()
        {
            Console.ForegroundColor = currentFg;
        }

        /*
        static void ApplyColors(string[] args)
        {
            if (args.Length == 0)
            {
                Console.WriteLine("usage: color <fg> [bg] or color default");
                return;
            }

            if (args[0].ToLower() == "default")
            {
                Console.ResetColor();
                return;
            }

            if (!int.TryParse(args[0], out int fg) || fg < 0 || fg > 15)
            {
                Console.WriteLine("invalid color :c");
                return;
            }

            Console.ForegroundColor = (ConsoleColor)fg;

            if (args.Length > 1)
            {
                if (!int.TryParse(args[1], out int bg) || bg < 0 || bg > 15)
                {
                    Console.WriteLine("invalid background color :c");
                    return;
                }

                Console.BackgroundColor = (ConsoleColor)bg;
            }
        }
        */

        static void ShowHelp()
        {
            Console.ForegroundColor = ConsoleColor.Yellow;
            Console.WriteLine("welcome to the help dialog1!1!11!1!!");
            Console.WriteLine("today imma show u all apps + their command lines");
            Console.WriteLine("that are available on this console app");
            Console.WriteLine("");
            Console.WriteLine("full app name - their cmd line");
            Console.WriteLine("about this simple console app - info / inf / i / aboutthisconsole / aboutthiscmd / abtthisconsole / abtthiscmd");
            Console.WriteLine("requirement to run this program - requirement / req / rq / require / required");
            Console.WriteLine("example dialog - example / ex / eg");
            Console.WriteLine("random number generator - rng");
            Console.WriteLine("simple calculator - calc / simplecalc");
            Console.WriteLine("clear screen - cls");
            Console.WriteLine("change colors - color <fg> [bg]");
            Console.WriteLine("reset colors - color default");
            Console.WriteLine("echo text - echo <text>");
            Console.WriteLine("time - time / when / watsthetime");
            Console.WriteLine("full time (date + time) - fulltime");
            Console.WriteLine("beep sound - beep");
            Console.WriteLine("live clock - watchtime");
            Console.WriteLine("fake crash screen - crash");
            Console.WriteLine("display ur current operating system - currentos / curos / os");
            Console.ForegroundColor = ConsoleColor.DarkYellow;
            Console.WriteLine("(spoiler alert: may only checks the real os, wsl linux would still");
            Console.WriteLine("be counted as windows)");
            Console.ForegroundColor = ConsoleColor.Yellow;
            Console.WriteLine("view logs - log / update (theres still more in tab autocorrect)");
            Console.WriteLine("exit app - exit (or press ctrl + c)");
            Console.ForegroundColor = ConsoleColor.DarkYellow;
            Console.WriteLine("(alternatively u can press ctrl + c to exit");
            Console.WriteLine("unless ur on windows without ctrl + shift + c copying method enabled)");
            Console.ForegroundColor = ConsoleColor.Yellow;
            Console.WriteLine("show disk storage n stuff - diskparty");
            Console.WriteLine("ping 1.1.1.1 and test network - netwatch");
            Console.WriteLine();
            Console.ForegroundColor = ConsoleColor.Red;
            Console.WriteLine("stuff that has stuff to do with files (be careful)");
            Console.ForegroundColor = ConsoleColor.Yellow;
            Console.WriteLine("browse through directory/folder - cd");
            Console.WriteLine("remove directory/folder - rmdir (or \"rm /rf\" to delete both files that are in it");
            Console.WriteLine("create directory/folder - mkdir");
            Console.WriteLine("create/modify a text document (or any other type) file - touch");
            Console.WriteLine("list out a directory/folder - ls / dir");
            Console.WriteLine("read out a files content - cat");
            Console.WriteLine("copy a source/file/folder to a destination - copy / cop / cope");
            Console.ForegroundColor = ConsoleColor.DarkRed;
            Console.WriteLine("(i am NOT be adding the cp command :sob::pray:)");
            Console.ForegroundColor = ConsoleColor.Yellow;
            Console.WriteLine("move a source/file/folder to a destination - move / mov / mv");
            Console.WriteLine("run a file - run / open / launch");
            Console.WriteLine("zip (or unzip) a file/folder - zip / unzip / (more in tab autocor)");
            Console.WriteLine("rename a file/folder - rename / ren / rn (ye, no, rN not rM");
            Console.WriteLine("view a files/sources has (sha-256) -  hash / sha256");
            Console.WriteLine();
            Console.ForegroundColor = ConsoleColor.Cyan;
            Console.WriteLine("commands that no one asked for");
            Console.ForegroundColor = ConsoleColor.Yellow;
            Console.WriteLine("show current directory (><[current dir]>$ ) - toggleShowDir (or lowercase: toggleshowdir)");
            Console.WriteLine("flip coin - coin / morecoins / flipcoin / flipacoin / headsntails / (more in tab autocorrect)");
            Console.WriteLine("show the \"fiscmd initialized...\" message earlier - initializefis / initfis");
            Console.WriteLine();
            Console.WriteLine("NEW COMMAND - import / importcmd\nit is said that bro can actually import SPECIAL commands :0");
            Console.ForegroundColor = ConsoleColor.Cyan;
            Console.WriteLine("how to use: import <specialcmd>");
            Console.WriteLine("<specialcmd> - special commands needed to import before using");
            Console.ForegroundColor = ConsoleColor.Yellow;
            Console.WriteLine("\nfor list of importable commands, type \"import\" and it'll shows the list :D");
            
            Console.ForegroundColor = ConsoleColor.Cyan;
            Console.WriteLine("\nmore are tab autocorrect! :D");

            ResForegroundColor();
        }

        static void RunRng()
        {
            Random rand = new Random();
            int num = rand.Next(0, 101);

            Console.ForegroundColor = ConsoleColor.Magenta;
            Console.WriteLine($"random number (0-100): {num}");
            ResForegroundColor();
        }

        static void RunCalc()
        {
            try
            {
                Console.Write("enter first number: ");
                double a = double.Parse(Console.ReadLine());

                Console.Write("operator (+ - * / ^): ");
                string op = Console.ReadLine();

                Console.Write("enter second number: ");
                double b = double.Parse(Console.ReadLine());

                double result = 0;

                switch (op)
                {
                    // basic math
                    case "+": result = a + b; break;
                    case "-": result = a - b; break;
                    case "*": result = a * b; break;
                    case "/": result = b != 0 ? a / b : throw new DivideByZeroException(); break;
                    // advanced math
                    case "^":
                    case "**": result = Math.Pow(a, b); break;
                    default:
                        Console.ForegroundColor = ConsoleColor.Red;
                        Console.WriteLine("invalid operator");
                        ResForegroundColor();
                        return;
                }

                Console.ForegroundColor = ConsoleColor.Blue;
                Console.WriteLine($"result: {result}");
                ResForegroundColor();
            }
            catch
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("calc error");
                ResForegroundColor();
            }
        }

        static void ShowExample()
        {
            Console.ForegroundColor = ConsoleColor.White;
            Console.BackgroundColor = ConsoleColor.DarkBlue;
            Console.WriteLine("   EXAMPLE   ");
            ResForegroundColor();
        }

        static void ShowTime()
        {
            var now = DateTime.Now;
            Console.ForegroundColor = ConsoleColor.Cyan;
            Console.WriteLine(now.ToString("HH:mm:ss"));
            ResForegroundColor();
        }

        static void ShowFullTime()
        {
            var now = DateTime.Now;
            Console.ForegroundColor = ConsoleColor.Cyan;
            Console.WriteLine(now.ToString("dddd, dd MMMM yyyy HH:mm:ss"));
            ResForegroundColor();
        }

        static void DoEcho(string input)
        {
            string[] parts = input.Split(' ');

            bool write = false;
            int delay = 25;
            int index = 1;

            void ShowUsage(string flag = null)
            {
                Console.ForegroundColor = ConsoleColor.Cyan;

                Console.WriteLine("usage: echo [/write] [/delay ms] [text]");

                if (flag == null)
                {
                    Console.WriteLine("[text] - the text needs to be inputted");
                    Console.WriteLine("[/write] - type the inputted text slowly instead of instanly prints them");
                    Console.WriteLine("[/delay ms] (requires [/write]) - delay before typing the next letter (counts in ms)");
                    Console.WriteLine("also [/delay ms] defaults at 25 ms");
                }
                else if (flag == "/write")
                {
                    Console.WriteLine("[/write] - type the inputted text slowly instead of instanly prints them");
                }
                else if (flag == "/delay")
                {
                    Console.WriteLine("[/delay ms] (requires [/write]) - delay before typing the next letter (counts in ms)");
                    Console.WriteLine("also [/delay ms] defaults at 25 ms");
                }

                ResForegroundColor();
            }

            // echo alone
            if (parts.Length == 1)
            {
                ShowUsage();
                return;
            }

            // parse flags
            while (index < parts.Length && parts[index].StartsWith("/"))
            {
                string flag = parts[index].ToLower();

                if (flag == "/write")
                {
                    write = true;
                    index++;
                }
                else if (flag == "/delay")
                {
                    if (index + 1 < parts.Length && int.TryParse(parts[index + 1], out int parsedDelay))
                    {
                        delay = parsedDelay;
                        index += 2;
                    }
                    else
                    {
                        ShowUsage("/delay");
                        return;
                    }
                }
                else
                {
                    index++; // ignore unknown flags
                }
            }

            // no text after flags
            if (index >= parts.Length)
            {
                if (write)
                    ShowUsage("/write");
                else
                    ShowUsage();

                return;
            }

            string text = string.Join(" ", parts, index, parts.Length - index);

            if (!write)
            {
                Console.WriteLine(text);
            }
            else
            {
                foreach (char c in text)
                {
                    Console.Write(c);
                    Thread.Sleep(delay);
                }
                Console.WriteLine();
            }
        }

        // original unused DoEcho() code
        /*
        static void DoEcho(string input)
        {
            if (input.Length <= 5)
            {
                Console.WriteLine();
                return;
            }

            string text = input.Substring(5);
            Console.WriteLine(text);
        }
        */

        // files modifier functions

        // mkdir (create folder)
        static void Mkdir(string[] args)
        {
            if (args.Length < 2)
            {
                Console.ForegroundColor = ConsoleColor.Cyan;
                Console.WriteLine("usage: mkdir <folder>");
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("i recommmend NOT adding a space in the name");
                return;
            }

            string path = Path.Combine(currentDir, string.Join(" ", args.Skip(1)));

            if (!Directory.Exists(path))
            {
                Directory.CreateDirectory(path);
                Console.ForegroundColor = ConsoleColor.Cyan;
                Console.WriteLine("directory/folder created :D");
            }
            else
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("directory/folder already exists bro");
            }

            ResForegroundColor();
        }

        // rmdir (remove folder)
        static void Rmdir(string[] args)
        {
            if (args.Length < 2)
            {
                Console.ForegroundColor = ConsoleColor.Cyan;
                Console.WriteLine("usage: rmdir <folder>");
                Console.Write("<folder> - the ");
                Console.ForegroundColor = ConsoleColor.Red;
                Console.Write("EMPTY ");
                Console.ForegroundColor = ConsoleColor.Cyan;
                Console.WriteLine("needed to delete");
                return;
            }

            string path = Path.Combine(currentDir, string.Join(" ", args.Skip(1)));

            if (!Directory.Exists(path))
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("directory/folder not found or invalid :c");
                return;
            }

            try
            {
                Directory.Delete(path); // non-recursive

                Console.ForegroundColor = ConsoleColor.Cyan;
                Console.WriteLine("directory/folder removed :D");
            }
            catch (IOException)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("bro this directory/folder aint empty");
                Console.WriteLine("use rm /rf if u REALLY mean it");
            }
            catch (Exception ex)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine($"failed to remove folder: {ex.Message}");
            }

            ResForegroundColor();
        }

        // rm (yes, /rf command too)
        static void Rm(string[] args)
        {
            if (args.Length < 2)
            {
                Console.ForegroundColor = ConsoleColor.Cyan;
                Console.WriteLine("usage: rm <file> OR rm /rf <folder>");
                Console.Write("/rf is to delete a whole entire folder ");
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("(be careful)");
                return;
            }

            // rm /rf folder (force delete)
            if (args[1] == "/rf")
            {
                if (args.Length < 3)
                {
                    Console.ForegroundColor = ConsoleColor.Cyan;
                    Console.WriteLine("usage: rm /rf <folder>");
                    Console.ForegroundColor = ConsoleColor.Red;
                    Console.WriteLine("BE CAREFUL WHILE USING THIS, it can PERMANENTLY delete an ENTIRE folder with contents in it");
                    Console.ForegroundColor = ConsoleColor.DarkRed;
                    Console.WriteLine("u have been warned...");
                    return;
                }

                string folderPath = Path.Combine(currentDir, string.Join(" ", args.Skip(2)));

                if (!Directory.Exists(folderPath))
                {
                    Console.ForegroundColor = ConsoleColor.Red;
                    Console.WriteLine("folder not found or invalid :c");
                    Console.WriteLine("maybe u forgr to use /rf??");
                    return;
                }

                Console.ForegroundColor = ConsoleColor.Red;
                Console.Write($"PERMANENTLY DELETE '{args[2]}' FR???? (y/n): ");
                string confirm = Console.ReadLine()?.ToLower() ?? "n";

                if (confirm != "y")
                {
                    Console.ForegroundColor = ConsoleColor.Cyan;
                    Console.WriteLine("oh cool u cancelled it :skull:");
                    return;
                }

                try
                {
                    Directory.Delete(folderPath, true);
                    Console.ForegroundColor = ConsoleColor.Red;
                    Console.WriteLine("directory/folder nuked, no more undo");
                }
                catch (Exception ex)
                {
                    Console.ForegroundColor = ConsoleColor.Red;
                    Console.WriteLine($"failed to delete directory/folder: {ex.Message}");
                }
            }
            else
            {
                // rm file
                string filePath = Path.Combine(currentDir, string.Join(" ", args.Skip(1)));

                if (!File.Exists(filePath))
                {
                    Console.ForegroundColor = ConsoleColor.Red;
                    Console.WriteLine("file not found or invalid :c");
                    return;
                }

                Console.ForegroundColor = ConsoleColor.Red;
                Console.Write($"PERMANENTLY DELETE '{args[1]}'????? (y/n): ");
                string confirm = Console.ReadLine()?.ToLower() ?? "n";

                if (confirm != "y")
                {
                    Console.ForegroundColor = ConsoleColor.Cyan;
                    Console.WriteLine("oh cool at least u cancelled it :skull:");
                    return;
                }

                try
                {
                    File.Delete(filePath);
                    Console.ForegroundColor = ConsoleColor.Red;
                    Console.WriteLine("file nuked, no more undo");
                }
                catch (Exception ex)
                {
                    Console.ForegroundColor = ConsoleColor.Red;
                    Console.WriteLine($"failed to delete file: {ex.Message}");
                }
            }

            ResForegroundColor();
        }

        // cd
        static void Cd(string[] args)
        {
            if (args.Length < 2)
            {
                Console.ForegroundColor = ConsoleColor.Cyan;
                Console.WriteLine($"current directory/folder: {currentDir}");
                ResForegroundColor();
                return;
            }

            string target = string.Join(" ", args.Skip(1));
            string newPath;

            if (target == "..")
            {
                newPath = Directory.GetParent(currentDir)?.FullName ?? currentDir;
            }
            else if (Path.IsPathRooted(target))
            {
                newPath = target;
            }
            else
            {
                newPath = Path.Combine(currentDir, target);
            }

            // ./ and ../ stuff
            newPath = Path.GetFullPath(newPath);

            if (Directory.Exists(newPath))
            {
                currentDir = newPath;
            }
            else
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("ts was barely even a directory/folder");
                ResForegroundColor();
            }
        }

        static void PrintPrompt()
        {
            if (Directory.Exists(currentDir) && showDir)
            {
                string normalized = Path.GetFullPath(currentDir).TrimEnd('\\');

                if (normalized.Equals("C:", StringComparison.OrdinalIgnoreCase))
                {
                    Console.Write("><[main os partition]>$ ");
                }
                else if (normalized.Equals("C:\\Windows", StringComparison.OrdinalIgnoreCase))
                {
                    Console.Write("><[important windows folder]>$ ");
                }
                else if (normalized.Equals("C:\\Windows\\System32", StringComparison.OrdinalIgnoreCase))
                {
                    Console.Write("><[important admin folder]>$ ");
                }
                /*
                else if (normalized.EndsWith("win-x64", StringComparison.OrdinalIgnoreCase))
                {
                    Console.Write("><[repos release win-x64 folder]>$ ");
                }
                */
                else if (normalized.EndsWith("repos", StringComparison.OrdinalIgnoreCase) || normalized.EndsWith("repo", StringComparison.OrdinalIgnoreCase))
                {
                    Console.Write("><[a repos folder]>$ ");
                }
                else if (normalized.Equals($"C:\\Users\\{Environment.UserName}", StringComparison.OrdinalIgnoreCase))
                {
                    Console.Write($"><[ur user folder, {Environment.UserName}]>$ ");
                }
                else if (normalized.Equals("C:\\Users", StringComparison.OrdinalIgnoreCase))
                {
                    Console.Write("><[user folder]>$ ");
                }
                else
                {
                    Console.Write($"><[{normalized}]>$ ");
                }
            }
            else
            {
                Console.Write("><>$ ");
            }
        }

        static void toggleShowDir()
        {
            showDir = !showDir;
        }

        static void Touch(string[] args)
        {
            if (args.Length < 2)
            {
                Console.ForegroundColor = ConsoleColor.Cyan;
                Console.WriteLine("usage: touch <filename>");
                Console.WriteLine("<filename> - name of the file needed to edit\n");
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("anonnoyingly flashes for every arrow key u press");
                Console.ForegroundColor = ConsoleColor.Cyan;
                Console.WriteLine("i hope ur all fine with ur suffering :sob::pray:");
                ResForegroundColor();
                return;
            }

            string filename = string.Join(" ", args.Skip(1)).Trim();
            string path = Path.Combine(currentDir, filename);

            List<string> lines = new List<string>();

            try
            {
                if (File.Exists(path))
                    lines = File.ReadAllLines(path).ToList();
                else
                    lines.Add("new file");

                ResForegroundColor();

                int cursorX = 0;
                int cursorY = 0;
                int scrollOffset = 0;

                Console.Clear();

                while (true)
                {
                    if (lines.Count == 0)
                        lines.Add("");

                    int winW = Console.WindowWidth;
                    int winH = Console.WindowHeight;
                    int visibleHeight = winH - 2;

                    // clamp cursor
                    cursorY = Math.Max(0, Math.Min(cursorY, lines.Count - 1));
                    cursorX = Math.Max(0, Math.Min(cursorX, lines[cursorY].Length));

                    Console.Clear();

                    // =========================
                    // render text area
                    // =========================
                    for (int i = 0; i < visibleHeight; i++)
                    {
                        int lineIndex = i + scrollOffset;

                        Console.SetCursorPosition(0, i);

                        string text = "";

                        if (lineIndex < lines.Count)
                            text = lines[lineIndex];

                        if (text.Length > winW)
                            text = text.Substring(0, winW);

                        Console.Write(text.PadRight(winW));
                    }

                    // =========================
                    // status bar
                    // =========================
                    Console.SetCursorPosition(0, winH - 2);
                    Console.ForegroundColor = ConsoleColor.DarkGray;

                    string status =
                        $"ESC = quit | CTRL + S = save | CTRL + X or :wq = save & quit | line {cursorY + 1}/{lines.Count}";

                    if (status.Length > winW)
                        status = status.Substring(0, winW);

                    Console.Write(status.PadRight(winW));
                    ResForegroundColor();

                    // =========================
                    // cursor draw
                    // =========================
                    int drawY = cursorY - scrollOffset;

                    if (drawY >= 0 && drawY < visibleHeight)
                    {
                        int drawX = Math.Min(cursorX, winW - 1);
                        Console.SetCursorPosition(drawX, drawY);
                    }

                    var key = Console.ReadKey(true);

                    // ESC quit no save
                    if (key.Key == ConsoleKey.Escape)
                    {
                        Console.Clear();
                        Console.WriteLine("exited without saving :skull:");
                        return;
                    }

                    // Ctrl+S save
                    if (key.Key == ConsoleKey.S &&
                        key.Modifiers.HasFlag(ConsoleModifiers.Control))
                    {
                        File.WriteAllLines(path, lines);

                        Console.SetCursorPosition(0, winH - 1);
                        Console.ForegroundColor = ConsoleColor.Cyan;

                        string msg = "[saved with Ctrl+S :D]";
                        if (msg.Length > winW)
                            msg = msg.Substring(0, winW);

                        Console.Write(msg.PadRight(winW));
                        ResForegroundColor();
                        continue;
                    }

                    // Ctrl+X save quit
                    if (key.Key == ConsoleKey.X &&
                        key.Modifiers.HasFlag(ConsoleModifiers.Control))
                    {
                        File.WriteAllLines(path, lines);

                        Console.Clear();
                        Console.ForegroundColor = ConsoleColor.Cyan;
                        Console.WriteLine("saved & exited :D");
                        ResForegroundColor();
                        return;
                    }

                    // ENTER
                    if (key.Key == ConsoleKey.Enter)
                    {
                        string currentLine = lines[cursorY];

                        if (currentLine.Trim() == ":wq")
                        {
                            lines.RemoveAt(cursorY);
                            File.WriteAllLines(path, lines);

                            Console.Clear();
                            Console.ForegroundColor = ConsoleColor.Cyan;
                            Console.WriteLine("saved :D");
                            ResForegroundColor();
                            return;
                        }

                        string left = currentLine.Substring(0, cursorX);
                        string right = currentLine.Substring(cursorX);

                        lines[cursorY] = left;
                        lines.Insert(cursorY + 1, right);

                        cursorY++;
                        cursorX = 0;
                    }

                    // BACKSPACE
                    else if (key.Key == ConsoleKey.Backspace)
                    {
                        if (cursorX > 0)
                        {
                            lines[cursorY] =
                                lines[cursorY].Remove(cursorX - 1, 1);
                            cursorX--;
                        }
                        else if (cursorY > 0)
                        {
                            int prevLen = lines[cursorY - 1].Length;

                            lines[cursorY - 1] += lines[cursorY];
                            lines.RemoveAt(cursorY);

                            cursorY--;
                            cursorX = prevLen;
                        }
                    }

                    // LEFT
                    else if (key.Key == ConsoleKey.LeftArrow)
                    {
                        if (cursorX > 0)
                            cursorX--;
                    }

                    // RIGHT
                    else if (key.Key == ConsoleKey.RightArrow)
                    {
                        if (cursorX < lines[cursorY].Length)
                            cursorX++;
                    }

                    // UP
                    else if (key.Key == ConsoleKey.UpArrow)
                    {
                        if (cursorY > 0)
                        {
                            cursorY--;
                            cursorX = Math.Min(cursorX, lines[cursorY].Length);
                        }
                    }

                    // DOWN
                    else if (key.Key == ConsoleKey.DownArrow)
                    {
                        if (cursorY < lines.Count - 1)
                        {
                            cursorY++;
                            cursorX = Math.Min(cursorX, lines[cursorY].Length);
                        }
                    }

                    // TEXT INPUT
                    else if (!char.IsControl(key.KeyChar))
                    {
                        lines[cursorY] =
                            lines[cursorY].Insert(cursorX, key.KeyChar.ToString());

                        cursorX++;
                    }

                    // =========================
                    // auto scroll
                    // =========================
                    if (cursorY >= scrollOffset + visibleHeight)
                        scrollOffset = cursorY - visibleHeight + 1;

                    if (cursorY < scrollOffset)
                        scrollOffset = cursorY;
                }
            }
            catch (Exception ex)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine($"failed to save file: {ex.Message}");
                ResForegroundColor();
            }
        }

        static void ls(string[] args)
        {
            try
            {
                // search for keywords
                if (args.Length >= 3 && args[1].ToLower() == "/search")
                {
                    string keyword = string.Join(" ", args.Skip(2));

                    Console.ForegroundColor = ConsoleColor.Cyan;
                    Console.WriteLine($"searching keyword for '{keyword}' in: '{currentDir}'...\n");

                    int found = 0;

                    // dirs
                    foreach (string dir in Directory.GetDirectories(currentDir, "*", SearchOption.AllDirectories))
                    {
                        string name = Path.GetFileName(dir);

                        if (name.Contains(keyword, StringComparison.OrdinalIgnoreCase))
                        {
                            Console.ForegroundColor = ConsoleColor.Yellow;
                            Console.WriteLine($"[DIR]  {dir}");
                            found++;
                        }
                    }

                    // files
                    foreach (string file in Directory.GetFiles(currentDir, "*", SearchOption.AllDirectories))
                    {
                        string name = Path.GetFileName(file);

                        if (name.Contains(keyword, StringComparison.OrdinalIgnoreCase))
                        {
                            long size = new FileInfo(file).Length;

                            Console.ForegroundColor = ConsoleColor.Yellow;
                            Console.WriteLine($"[FILE] {file} ({size} bytes)");
                            found++;
                        }
                    }

                    if (found == 0)
                    {
                        Console.ForegroundColor = ConsoleColor.Yellow;
                        Console.WriteLine("found nothing");
                    }
                    else
                    {
                        Console.ForegroundColor = ConsoleColor.Cyan;
                        Console.WriteLine($"\nfound {found} result(s) :D");
                    }

                    ResForegroundColor();
                    return;
                }

                // normal ls
                string[] dirs = Directory.GetDirectories(currentDir);
                string[] files = Directory.GetFiles(currentDir);

                Console.ForegroundColor = ConsoleColor.Cyan;
                Console.WriteLine($"listing: {currentDir}\n");

                // list directories first
                foreach (string dir in dirs)
                {
                    string name = Path.GetFileName(dir);

                    Console.ForegroundColor = ConsoleColor.Yellow;
                    Console.WriteLine($"[DIR]  {name}");
                }

                // then files
                foreach (string file in files)
                {
                    string name = Path.GetFileName(file);

                    long size = new FileInfo(file).Length;

                    Console.ForegroundColor = ConsoleColor.Yellow;
                    Console.WriteLine($"[FILE] {name} ({size} bytes)");
                }

                // empty check
                if (dirs.Length == 0 && files.Length == 0)
                {
                    Console.ForegroundColor = ConsoleColor.Red;
                    Console.WriteLine("this folder empty as hell");
                    Console.WriteLine("tf are u expecting me to do :sob:");
                }
            }
            catch (UnauthorizedAccessException)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("restricted folder, get out :skull::wilted-rose:");
            }
            catch (Exception ex)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine($"failed to list folder: {ex.Message}");
            }

            ResForegroundColor();
        }

        static void Cat(string[] args)
        {
            if (args.Length < 2)
            {
                Console.ForegroundColor = ConsoleColor.Cyan;
                Console.WriteLine("usage: cat <filename>");
                Console.WriteLine("<filename> - the file needed to echo out");
                return;
            }

            // combine currentDir with the filename to get the full path
            string path = Path.Combine(currentDir, string.Join(" ", args.Skip(1)));

            if (File.Exists(path))
            {
                try
                {
                    string content = File.ReadAllText(path);
                    Console.ForegroundColor = ConsoleColor.Yellow;
                    Console.WriteLine(content);
                }
                catch (Exception ex)
                {
                    Console.ForegroundColor = ConsoleColor.Red;
                    Console.WriteLine($"failed to read the file: {ex.Message}");
                }
            }
            else
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("file not found/invalid :c");
            }

            ResForegroundColor();
        }

        static void Copy(string[] args)
        {
            if (args.Length < 3)
            {
                Console.ForegroundColor = ConsoleColor.Cyan;
                Console.WriteLine("usage: copy/cop/cope [-r] <source/file> <destination>");
                Console.WriteLine("[-r] - copy the entire folder, including its (sub)folders");
                Console.WriteLine("<source/file> - the source/file needed to copy");
                Console.WriteLine("<destination> - the destination needed to indicate where should the copied source/file go");
                ResForegroundColor();
                return;
            }

            bool recursive = args.Contains("-r");
            var filtered = args.Where(a => a != "-r").ToArray();

            if (filtered.Length < 3)
            {
                Console.ForegroundColor = ConsoleColor.Cyan;
                Console.WriteLine("usage: copy/cop/cope [-r] <source> <destination>");
                Console.WriteLine("[-r] - copy the entire folder, including its (sub)folders");
                Console.WriteLine("<source/file> - the source/file needed to copy");
                Console.WriteLine("<destination> - the destination needed to indicate where should the copied source/file go");
                ResForegroundColor();
                return;
            }

            string sourcePath = Path.GetFullPath(Path.Combine(currentDir, filtered[1]));
            string destPath = Path.GetFullPath(Path.Combine(currentDir, filtered[2]));

            try
            {
                // SAME FILE CHECK
                if (sourcePath == destPath)
                {
                    Console.ForegroundColor = ConsoleColor.Yellow;
                    Console.WriteLine("source and destination are the same :skull:");
                    ResForegroundColor();
                    return;
                }

                // FILE
                if (File.Exists(sourcePath))
                {
                    if (Directory.Exists(destPath))
                        destPath = Path.Combine(destPath, Path.GetFileName(sourcePath));

                    File.Copy(sourcePath, destPath, true);

                    Console.ForegroundColor = ConsoleColor.Cyan;
                    Console.WriteLine("1 file copied :D");
                    ResForegroundColor();
                    return;
                }

                // DIRECTORY
                if (Directory.Exists(sourcePath))
                {
                    if (!recursive)
                    {
                        Console.ForegroundColor = ConsoleColor.Red;
                        Console.WriteLine("use -r to copy directories");
                        ResForegroundColor();
                        return;
                    }

                    int copied = 0;
                    int skipped = 0;

                    CopyDirectory(sourcePath, destPath, ref copied, ref skipped);

                    Console.ForegroundColor = ConsoleColor.Cyan;
                    Console.WriteLine($"{copied} files copied, {skipped} skipped :D");
                    ResForegroundColor();
                    return;
                }

                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("source not found :c");
                ResForegroundColor();
            }
            catch (Exception ex)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine($"copy failed: {ex.Message}");
                ResForegroundColor();
            }
        }

        // helper for recursive copy
        static void CopyDirectory(string sourceDir, string destDir, ref int copied, ref int skipped)
        {
            try
            {
                Directory.CreateDirectory(destDir);
            }
            catch
            {
                skipped++;
                return;
            }

            // FILES
            foreach (string file in Directory.GetFiles(sourceDir))
            {
                string destFile = Path.Combine(destDir, Path.GetFileName(file));

                try
                {
                    // avoid copying onto itself
                    if (Path.GetFullPath(file) == Path.GetFullPath(destFile))
                    {
                        skipped++;
                        continue;
                    }

                    File.Copy(file, destFile, true);
                    copied++;
                }
                catch
                {
                    skipped++;
                }
            }

            // SUBDIRS
            foreach (string dir in Directory.GetDirectories(sourceDir))
            {
                string destSubDir = Path.Combine(destDir, Path.GetFileName(dir));

                try
                {
                    CopyDirectory(dir, destSubDir, ref copied, ref skipped);
                }
                catch
                {
                    skipped++;
                }
            }
        }

        // move function
        static void Move(string[] args)
        {
            if (args.Length < 3)
            {
                Console.ForegroundColor = ConsoleColor.Cyan;
                Console.WriteLine("usage: move/mov/mv [-r] <source/file> <destination>");
                Console.WriteLine("[-r] - move the entire folder, including its (sub)folders");
                Console.WriteLine("<source/file> - the source/file needed to move");
                Console.WriteLine("<destination> - the destination needed to indicate where should the source/file move to");
                ResForegroundColor();
                return;
            }

            bool recursive = args.Contains("-r");
            var filtered = args.Where(a => a != "-r").ToArray();

            if (filtered.Length < 3)
            {
                Console.ForegroundColor = ConsoleColor.Cyan;
                Console.WriteLine("usage: move/mov/mv [-r] <source/file> <destination>");
                Console.WriteLine("[-r] - move the entire folder, including its (sub)folders");
                Console.WriteLine("<source/file> - the source/file needed to move");
                Console.WriteLine("<destination> - the destination needed to indicate where should the source/file move to");
                ResForegroundColor();
                return;
            }

            string sourcePath = Path.GetFullPath(Path.Combine(currentDir, filtered[1]));
            string destPath = Path.GetFullPath(Path.Combine(currentDir, filtered[2]));

            try
            {
                // SAME PATH CHECK
                if (sourcePath == destPath)
                {
                    Console.ForegroundColor = ConsoleColor.Red;
                    Console.WriteLine("source and destination are the same :skull:");
                    ResForegroundColor();
                    return;
                }

                // FILE
                if (File.Exists(sourcePath))
                {
                    if (Directory.Exists(destPath))
                        destPath = Path.Combine(destPath, Path.GetFileName(sourcePath));

                    try
                    {
                        File.Move(sourcePath, destPath, true);
                    }
                    catch
                    {
                        // fallback (cross-drive etc.)
                        File.Copy(sourcePath, destPath, true);
                        File.Delete(sourcePath);
                    }

                    Console.ForegroundColor = ConsoleColor.Cyan;
                    Console.WriteLine("1 file moved :D");
                    ResForegroundColor();
                    return;
                }

                // DIRECTORY
                if (Directory.Exists(sourcePath))
                {
                    if (!recursive)
                    {
                        Console.ForegroundColor = ConsoleColor.Red;
                        Console.WriteLine("use -r to move directories");
                        ResForegroundColor();
                        return;
                    }

                    int moved = 0;
                    int skipped = 0;

                    MoveDirectory(sourcePath, destPath, ref moved, ref skipped);

                    Console.ForegroundColor = ConsoleColor.Cyan;
                    Console.WriteLine($"{moved} items moved, {skipped} skipped :D");
                    ResForegroundColor();
                    return;
                }

                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("source not found :c");
                ResForegroundColor();
            }
            catch (Exception ex)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine($"move failed: {ex.Message}");
                ResForegroundColor();
            }
        }

        // helper for recursive movement
        static void MoveDirectory(string sourceDir, string destDir, ref int moved, ref int skipped)
        {
            try
            {
                Directory.CreateDirectory(destDir);
            }
            catch
            {
                skipped++;
                return;
            }

            // FILES
            foreach (string file in Directory.GetFiles(sourceDir))
            {
                string destFile = Path.Combine(destDir, Path.GetFileName(file));

                try
                {
                    if (Path.GetFullPath(file) == Path.GetFullPath(destFile))
                    {
                        skipped++;
                        continue;
                    }

                    try
                    {
                        File.Move(file, destFile, true);
                    }
                    catch
                    {
                        // fallback
                        File.Copy(file, destFile, true);
                        File.Delete(file);
                    }

                    moved++;
                }
                catch
                {
                    skipped++;
                }
            }

            // SUBDIRS
            foreach (string dir in Directory.GetDirectories(sourceDir))
            {
                string destSubDir = Path.Combine(destDir, Path.GetFileName(dir));

                try
                {
                    MoveDirectory(dir, destSubDir, ref moved, ref skipped);

                    // remove empty source dir after moving
                    try { Directory.Delete(dir, true); } catch { }
                }
                catch
                {
                    skipped++;
                }
            }

            // delete root source at end
            try { Directory.Delete(sourceDir, true); } catch { }
        }

        static void RunCommand(string[] args)
        {
            try
            {
                if (args.Length < 2)
                {
                    Console.ForegroundColor = ConsoleColor.Cyan;
                    Console.WriteLine("usage: run/open/launch [args] <file>");
                    Console.WriteLine("u can replace [args] with:");
                    Console.WriteLine("/system32 (or /sys32) - run files only from system32");
                    Console.WriteLine("/website - launches webpage/website instead of files");
                    Console.WriteLine("or leave empty to run files normally");
                    return;
                }

                // WEBSITE MODE
                if (args[1].ToLower() == "/website")
                {
                    if (args.Length < 3)
                    {
                        Console.ForegroundColor = ConsoleColor.Red;
                        Console.WriteLine("u forgot the website link bro");
                        return;
                    }

                    string website = args[2];

                    if (!website.StartsWith("http://") && !website.StartsWith("https://"))
                    {
                        Console.ForegroundColor = ConsoleColor.Cyan;
                        Console.WriteLine("u forgr the https:// thing brah");
                        Console.WriteLine("press enter or y to auto add");

                        string confirm = Console.ReadLine()?.ToLower() ?? "n";

                        if (confirm == "y" || confirm == "")
                        {
                            website = "https://" + website;
                        }
                    }

                    Process.Start(new ProcessStartInfo()
                    {
                        FileName = website,
                        UseShellExecute = true
                    });

                    Console.ForegroundColor = ConsoleColor.Cyan;
                    Console.WriteLine("website opened successfully :D");
                    return;
                }

                string target = "";

                // SYS32 MODE
                if (args[1].ToLower() == "/sys32" || args[1].ToLower() == "/system32")
                {
                    if (args.Length < 3)
                    {
                        Console.ForegroundColor = ConsoleColor.Red;
                        Console.WriteLine("u forgot app name bro");
                        return;
                    }

                    string appName = args[2];


                    // auto add .exe if no extension
                    if (Path.GetExtension(appName) == "")
                    {
                        Console.ForegroundColor = ConsoleColor.Cyan;
                        Console.WriteLine("u forgr the .exe thing brah");
                        Console.WriteLine("press enter or y to auto add");

                        string confirm1 = Console.ReadLine()?.ToLower() ?? "n";

                        if (confirm1 == "" || confirm1 == "y")
                        {
                            appName += ".exe";
                        }
                    }

                    target = Path.Combine(sys32fol, appName);
                }
                else
                {
                    // NORMAL MODE
                    target = args[1];

                    if (!Path.IsPathRooted(target))
                    {
                        target = Path.Combine(currentDir, target);
                    }
                }

                if (!File.Exists(target))
                {
                    Console.ForegroundColor = ConsoleColor.Red;
                    Console.WriteLine("file not found/invalid :c");
                    return;
                }

                Process.Start(new ProcessStartInfo()
                {
                    FileName = target,
                    UseShellExecute = true
                });

                Console.ForegroundColor = ConsoleColor.Cyan;
                Console.WriteLine("launched successfully :D");
            }
            catch (Exception ex)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("failed to launch: " + ex.Message);
            }

            ResForegroundColor();
        }

        // more goofy miscs

        static void ShowHistory()
        {
            Console.ForegroundColor = ConsoleColor.Cyan;

            if (history.Count == 0)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("no history yet :c");
                ResForegroundColor();
                return;
            }

            for (int i = 0; i < history.Count; i++)
                Console.WriteLine($"{i + 1}. {history[i]}");

            ResForegroundColor();
        }

        static void ClearHistory()
        {
            history.Clear();
            historyIndex = -1;

            Console.ForegroundColor = ConsoleColor.DarkRed;
            Console.WriteLine("ur entire history is nuked, no more undo");
            ResForegroundColor();
        }

        static void ShowVersion()
        {
            Console.ForegroundColor = ConsoleColor.Yellow;
            Console.WriteLine("fiscmd v1.6 beta ><>");
            /*
            Console.ForegroundColor = ConsoleColor.DarkYellow;
            Console.WriteLine("(LETS GO FINAL V2 WE COOKED)");
            */
            Console.ForegroundColor = ConsoleColor.Yellow;
            Console.Write("\nrequirement: ");
            Console.ForegroundColor = ConsoleColor.DarkMagenta;
            Console.WriteLine(".NET 6.0 LTS");
            Console.WriteLine();
            Console.ForegroundColor = ConsoleColor.DarkYellow;
            Console.WriteLine("pov: me when the");
            ResForegroundColor();
        }

        static void SetTitle(string[] args)
        {
            if (args.Length < 2)
            {
                Console.ForegroundColor = ConsoleColor.Cyan;
                Console.WriteLine("usage: title <text>");
                Console.WriteLine("<text> is the new console title");
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("whatever u do... DONT USE CURSED CHARACTERS FOR TS :skull:");
                ResForegroundColor();
                return;
            }

            Console.Title = string.Join(" ", args.Skip(1));

            Console.ForegroundColor = ConsoleColor.Cyan;
            Console.WriteLine("title changed :D");
            ResForegroundColor();
        }

        static void PrintFis()
        {
            ResForegroundColor();
            Console.WriteLine("><> look de fis :0");
            ResForegroundColor(); // cant scream without ts :sob::wilted-rose:
        }

        static void FlipCoin()
        {
            Random r = new Random();

            Console.ForegroundColor = ConsoleColor.Cyan;
            Console.WriteLine(r.Next(2) == 0 ? "heads" : "tails");
            ResForegroundColor();
        }

        static void SysInfo()
        {
            Console.ForegroundColor = ConsoleColor.Yellow;
            Console.WriteLine($"machine: {Environment.MachineName}");
            Console.WriteLine($"user: {Environment.UserName}");
            Console.WriteLine($"os: {Environment.OSVersion}");
            Console.Write($"64 bit os yes or no: ");
            if (Environment.Is64BitOperatingSystem)
            {
                Console.WriteLine("yes");
            }
            else
            {
                Console.WriteLine("nah");
            }

            Console.WriteLine($".net: {Environment.Version}");
            ResForegroundColor();
        }

        static void MemInfo()
        {
            long mem = GC.GetTotalMemory(false);

            Console.ForegroundColor = ConsoleColor.Yellow;
            Console.WriteLine($"total memory: {mem} KB");
            Console.WriteLine($"managed memory: {mem / 1024} KB");
            ResForegroundColor();
        }

        static void Benchmark()
        {
            var sw = System.Diagnostics.Stopwatch.StartNew();

            for (int i = 0; i < 1000000; i++)
            {
                int x = i * i;
            }

            sw.Stop();

            Console.ForegroundColor = ConsoleColor.Cyan;
            Console.WriteLine($"benchmark done in {sw.ElapsedMilliseconds} ms");
            ResForegroundColor();
        }


        // more file interaction

        static void TreeCommand(string[] args)
        {
            string target = currentDir;

            Console.ForegroundColor = ConsoleColor.Cyan;
            Console.WriteLine($"hollup, creating tree for the directory: {currentDir}");
            Console.WriteLine();

            if (args.Length >= 2)
                target = Path.Combine(currentDir, args[1]);

            if (!Directory.Exists(target))
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("folder not found or invalid :c");
                ResForegroundColor();
                return;
            }

            Console.ForegroundColor = ConsoleColor.Cyan;
            Console.WriteLine(Path.GetFileName(target));
            Tree(target);

            ResForegroundColor();
        }

        static void Tree(string path, string indent = "")
        {
            string[] dirs = Directory.GetDirectories(path);
            string[] files = Directory.GetFiles(path);

            Console.ForegroundColor = ConsoleColor.Yellow;

            foreach (string dir in dirs)
            {
                Console.WriteLine(indent + "├─ " + Path.GetFileName(dir));
                Tree(dir, indent + "│  ");
            }

            foreach (string file in files)
            {
                Console.WriteLine(indent + "└─ " + Path.GetFileName(file));
            }

            ResForegroundColor();
        }

        static void Rename(string[] args)
        {
            if (args.Length < 3)
            {
                Console.ForegroundColor = ConsoleColor.Cyan;
                Console.WriteLine("usage: rename <old> <new>");
                Console.WriteLine("<old> is source file/folder");
                Console.WriteLine("<new> is new name");
                ResForegroundColor();
                return;
            }

            string oldPath = Path.Combine(currentDir, args[1]);
            string newPath = Path.Combine(currentDir, args[2]);

            try
            {
                if (File.Exists(oldPath))
                    File.Move(oldPath, newPath);
                else if (Directory.Exists(oldPath))
                    Directory.Move(oldPath, newPath);
                else
                {
                    Console.ForegroundColor = ConsoleColor.Red;
                    Console.WriteLine("source not found :c");
                    ResForegroundColor();
                    return;
                }

                Console.ForegroundColor = ConsoleColor.Cyan;
                Console.WriteLine("renamed :D");
            }
            catch (Exception ex)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine($"failed to rename the file/source: {ex.Message}");
            }

            ResForegroundColor();
        }

        static void ZipFolder(string[] args)
        {
            if (args.Length < 2)
            {
                Console.ForegroundColor = ConsoleColor.Cyan;
                Console.WriteLine("usage: zip <folder>");
                Console.WriteLine("<folder> is the folder needed to compress/zip");
                ResForegroundColor();
                return;
            }

            string folder = Path.Combine(currentDir, args[1]);
            string zip = folder + ".zip";

            try
            {
                ZipFile.CreateFromDirectory(folder, zip);

                Console.ForegroundColor = ConsoleColor.Cyan;
                Console.WriteLine("zipped :D");
            }
            catch (Exception ex)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine($"zip failed: {ex.Message}");
            }

            ResForegroundColor();
        }

        static void UnzipFile(string[] args)
        {
            if (args.Length < 2)
            {
                Console.ForegroundColor = ConsoleColor.Cyan;
                Console.WriteLine("usage: unzip <file.zip>");
                Console.WriteLine("<file.zip> is the archive to extract");
                ResForegroundColor();
                return;
            }

            string zip = Path.Combine(currentDir, args[1]);
            string outDir = zip + "_unzipped";

            try
            {
                ZipFile.ExtractToDirectory(zip, outDir);

                Console.ForegroundColor = ConsoleColor.Cyan;
                Console.WriteLine("unzipped :D");
            }
            catch (Exception ex)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine($"unzip failed: {ex.Message}");
            }

            ResForegroundColor();
        }

        static void HashFile(string[] args)
        {
            if (args.Length < 2)
            {
                Console.ForegroundColor = ConsoleColor.Cyan;
                Console.WriteLine("usage: hash <file> [/sha int]");
                Console.WriteLine("<file> is the file to hash");
                Console.WriteLine("[/sha int] - 1, 256, 384, 512 (default = 256)");
                ResForegroundColor();
                return;
            }

            int shaType = 256;
            string filePath = "";

            // parse args
            for (int i = 1; i < args.Length; i++)
            {
                if (args[i].ToLower() == "/sha" && i + 1 < args.Length)
                {
                    int.TryParse(args[i + 1], out shaType);
                    i++; // skip next arg
                }
                else
                {
                    // collect file path (supports spaces)
                    if (filePath.Length > 0)
                        filePath += " ";
                    filePath += args[i];
                }
            }

            string path = Path.Combine(currentDir, filePath);

            try
            {
                byte[] hash;

                using var stream = File.OpenRead(path);

                Console.ForegroundColor = ConsoleColor.Cyan;

                switch (shaType)
                {
                    case 1:
                        using (var sha1 = System.Security.Cryptography.SHA1.Create())
                            hash = sha1.ComputeHash(stream);
                        break;

                    case 384:
                        using (var sha384 = System.Security.Cryptography.SHA384.Create())
                            hash = sha384.ComputeHash(stream);
                        break;

                    case 512:
                        using (var sha512 = System.Security.Cryptography.SHA512.Create())
                            hash = sha512.ComputeHash(stream);
                        break;

                    default:
                        using (var sha256 = System.Security.Cryptography.SHA256.Create())
                            hash = sha256.ComputeHash(stream);
                        break;
                }

                Console.ForegroundColor = ConsoleColor.Cyan;
                Console.WriteLine(BitConverter.ToString(hash).Replace("-", ""));
            }
            catch (Exception ex)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine($"hash failed: {ex.Message}");
            }

            ResForegroundColor();
        }

        static void ShowUpdate()
        {
            Console.ForegroundColor = ConsoleColor.Yellow;
            TypeWrite("v1.6 beta logs (press any key for each next log ok):\n");
            Console.ReadKey(true);
            TypeWrite("- updated the importable command \"fisdraw\" (un/redo support + erase 1 pixel + save/load)");
            Console.ReadKey(true);
            TypeWrite("- apoligized for confusing yall with \"fisuni\" in v1.5 :sob::wilted-flower:");
            Console.ReadKey(true);
            TypeWrite("- ", 10, false);
            Console.ForegroundColor = ConsoleColor.Red;
            TypeWrite("completely rewrote fiscmd command input system", 10, false);
            Console.ForegroundColor = ConsoleColor.Cyan;
            TypeWrite("\nso now that the cursor can move around using arrow keys and ofc Home/End keys");
            Console.ReadKey(true);
            Console.ForegroundColor = ConsoleColor.Yellow;
            TypeWrite("- NEW COMMANDS: netwatch, diskparty (seek \"help\" for more brotha)");
            TypeWrite("- NEW IMPORTABLE COMMAND: fisstars (seek \"import\" for more brotha)");

            Console.ReadKey(true);
            TypeWrite("- thats it lmao");
            Console.ReadKey(true);
            Console.ForegroundColor = ConsoleColor.Blue;
            TypeWrite("\nfor more logs, https://discord.gg/C4g2RgYr2g");
            TypeWrite("visit the same website now?? (default when pressing [ENTER] = no)");
            if (showDir)
            {
                Console.ForegroundColor = ConsoleColor.Cyan;
                Console.Write("><[answer]> ");
            }
            else
            {
                ResForegroundColor();
            }

            string confirm = Console.ReadLine()?.ToLower() ?? "n";

            if (confirm == "y")
            {
                Process.Start(new ProcessStartInfo()
                {
                    FileName = "https://discord.gg/C4g2RgYr2g",
                    UseShellExecute = true
                });
                Console.ForegroundColor = ConsoleColor.Cyan;
                TypeWrite("we basically launched the browser for u :D");
                Console.ReadKey(true);
            }

            Console.ForegroundColor = ConsoleColor.Cyan;
            TypeWrite("\ntysm for using my console app :D");
            Console.ForegroundColor = ConsoleColor.DarkCyan;
            TypeWrite("press any key to return");
            Console.ReadKey(true);
            return;
        }

        // fistop
        static void Taskmgr(string[] args)
        {
            try
            {
                // fistop /watch
                // fistop /watch chrome
                // fistop /live
                // fistop /live roblox
                if (args.Length > 1 &&
                    (args[1].ToLower() == "/watch" || args[1].ToLower() == "/live"))
                {
                    string keyword = "";

                    // optional keyword
                    if (args.Length > 2)
                    {
                        keyword = args[2].ToLower();
                    }

                    Console.ForegroundColor = ConsoleColor.Cyan;

                    if (keyword != "")
                    {
                        Console.WriteLine($"watching for '{keyword}' processes...");
                    }
                    else
                    {
                        Console.WriteLine("watching ALL processes...");
                    }

                    Console.WriteLine("press any key to stop watching\n");

                    ResForegroundColor();

                    while (!Console.KeyAvailable)
                    {
                        Console.Clear();

                        var watchProcesses = Process.GetProcesses()
                            .Where(p =>
                            {
                                try
                                {
                                    if (keyword == "")
                                        return true;

                                    return p.ProcessName.ToLower().Contains(keyword);
                                }
                                catch
                                {
                                    return false;
                                }
                            })
                            .OrderBy(p => p.ProcessName)
                            .ToArray();

                        Console.ForegroundColor = ConsoleColor.Cyan;

                        if (keyword != "")
                        {
                            Console.WriteLine($"live process watcher for '{keyword}'");
                        }
                        else
                        {
                            Console.WriteLine("live process watcher");
                        }

                        Console.WriteLine($"updated at: {DateTime.Now}");
                        Console.WriteLine("press any key to exit");
                        Console.WriteLine("\nname                 pid");
                        Console.WriteLine("------------------------");

                        Console.ForegroundColor = ConsoleColor.Yellow;

                        if (watchProcesses.Length <= 0)
                        {
                            Console.ForegroundColor = ConsoleColor.Red;
                            Console.WriteLine("no matching processes found :sob::pray:");
                        }

                        foreach (var proc in watchProcesses)
                        {
                            try
                            {
                                Console.WriteLine($"{proc.ProcessName.PadRight(20)} {proc.Id}");
                            }
                            catch { }
                        }

                        Thread.Sleep(1000);
                    }

                    Console.ReadKey(true);

                    Console.ForegroundColor = ConsoleColor.Cyan;
                    Console.WriteLine("\nstopped watching process :D");
                    ResForegroundColor();
                    return;
                }

                var processes = Process.GetProcesses();

                // fistop /search chrome
                if (args.Length > 1 && args[1].ToLower() == "/search")
                {
                    if (args.Length < 3)
                    {
                        Console.ForegroundColor = ConsoleColor.Red;
                        Console.WriteLine("u forgr the keyword bro");
                        ResForegroundColor();
                        return;
                    }

                    string keyword = args[2].ToLower();

                    processes = processes
                        .Where(p =>
                        {
                            try
                            {
                                return p.ProcessName.ToLower().Contains(keyword);
                            }
                            catch
                            {
                                return false;
                            }
                        })
                        .ToArray();
                }

                // fistop /searchpid 1234
                else if (args.Length > 1 && args[1].ToLower() == "/searchpid")
                {
                    if (args.Length < 3)
                    {
                        Console.ForegroundColor = ConsoleColor.Red;
                        Console.WriteLine("u forgr the pid bro");
                        ResForegroundColor();
                        return;
                    }

                    if (!int.TryParse(args[2], out int targetPid))
                    {
                        Console.ForegroundColor = ConsoleColor.Red;
                        Console.WriteLine("ts was barely even a pid");
                        ResForegroundColor();
                        return;
                    }

                    processes = processes
                        .Where(p => p.Id == targetPid)
                        .ToArray();
                }

                Console.ForegroundColor = ConsoleColor.Cyan;
                Console.WriteLine("name                 pid");
                Console.WriteLine("------------------------");

                Console.ForegroundColor = ConsoleColor.Yellow;

                foreach (var proc in processes.OrderBy(p => p.ProcessName))
                {
                    try
                    {
                        Console.WriteLine($"{proc.ProcessName.PadRight(20)} {proc.Id}");
                    }
                    catch { }
                }
            }
            catch (Exception ex)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine($"failed to get processes: {ex.Message}");
            }

            ResForegroundColor();
        }

        // fiskill
        static void KillProc(string[] args)
        {
            if (args.Length < 2)
            {
                Console.ForegroundColor = ConsoleColor.Cyan;
                Console.WriteLine("usage: fiskill <pid> (/search [keyword])");
                Console.WriteLine("<pid> - the exact process' pid to terminate");
                Console.WriteLine("(/search [keyword]) - optional switch to search the process' keyword before terminating it");
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("\nbe careful when using this...");
                ResForegroundColor();
                return;
            }

            try
            {
                // fiskill /search chrome
                if (args[1].ToLower() == "/search")
                {
                    if (args.Length < 3)
                    {
                        Console.ForegroundColor = ConsoleColor.Red;
                        Console.WriteLine("u forgr the keyword bro");
                        ResForegroundColor();
                        return;
                    }

                    string keyword = args[2].ToLower();

                    var matches = Process.GetProcesses()
                        .Where(p =>
                        {
                            try
                            {
                                return p.ProcessName.ToLower().Contains(keyword);
                            }
                            catch
                            {
                                return false;
                            }
                        });

                    Console.ForegroundColor = ConsoleColor.Yellow;

                    foreach (var proc in matches)
                    {
                        try
                        {
                            Console.WriteLine($"{proc.ProcessName} (PID: {proc.Id})");
                        }
                        catch { }
                    }

                    Console.ForegroundColor = ConsoleColor.Cyan;
                    Console.Write("\nenter pid to terminate: ");

                    string pidInput = Console.ReadLine();

                    if (!int.TryParse(pidInput, out int searchedPid))
                    {
                        Console.ForegroundColor = ConsoleColor.Red;
                        Console.WriteLine("ts was barely even a pid");
                        ResForegroundColor();
                        return;
                    }

                    Process target = Process.GetProcessById(searchedPid);

                    Console.ForegroundColor = ConsoleColor.Red;
                    Console.Write($"PERMANENTLY terminate '{target.ProcessName}'????? (y/n): ");

                    string confirm = Console.ReadLine()?.ToLower() ?? "n";

                    if (confirm != "y")
                    {
                        Console.ForegroundColor = ConsoleColor.Cyan;
                        Console.WriteLine("oh cool u cancelled it :skull:");
                        ResForegroundColor();
                        return;
                    }

                    target.Kill(true);

                    Console.ForegroundColor = ConsoleColor.DarkRed;
                    Console.WriteLine("process sent to the shadow realm");
                    ResForegroundColor();
                    return;
                }

                // fiskill 1234
                if (!int.TryParse(args[1], out int pid))
                {
                    Console.ForegroundColor = ConsoleColor.Red;
                    Console.WriteLine("ts was barely even a pid");
                    ResForegroundColor();
                    return;
                }

                Process procToKill = Process.GetProcessById(pid);

                Console.ForegroundColor = ConsoleColor.Red;
                Console.Write($"TERMINATE '{procToKill.ProcessName}' FR????? (y/n): ");

                string confirm2 = Console.ReadLine()?.ToLower() ?? "n";

                if (confirm2 != "y")
                {
                    Console.ForegroundColor = ConsoleColor.Cyan;
                    Console.WriteLine("oh cool u cancelled it :skull:");
                    ResForegroundColor();
                    return;
                }

                procToKill.Kill(true);

                Console.ForegroundColor = ConsoleColor.DarkRed;
                Console.WriteLine("process sent to the shadow realm");
            }
            catch (Exception ex)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine($"failed to terminate the process: {ex.Message}");
            }

            ResForegroundColor();
        }
        
        // netwatch
        static void NetWatch()
        {
            Ping ping = new Ping();
            Random r = new Random();

            Console.WriteLine("press any key to stop...\n");

            while (!Console.KeyAvailable)
            {
                try
                {
                    PingReply reply = ping.Send("1.1.1.1");

                    Console.Clear();

                    if (reply.Status == IPStatus.Success)
                    {
                        long ms = reply.RoundtripTime;

                        Console.ForegroundColor = ConsoleColor.Cyan;
                        Console.WriteLine($"PING: {ms}ms");

                        Console.ForegroundColor = ConsoleColor.Green;
                        Console.Write("GRAPH: ");

                        for (int i = 0; i < 20; i++)
                        {
                            Console.Write(r.Next(0, 2) == 0 ? "-" : "_");
                        }

                        Console.WriteLine();
                    }
                    else
                    {
                        Console.ForegroundColor = ConsoleColor.Red;
                        Console.WriteLine("bros internet died :sob:");
                    }
                }
                catch
                {
                    Console.ForegroundColor = ConsoleColor.Red;
                    Console.WriteLine("router exploded");
                }

                Thread.Sleep(1000);
            }

            Console.ReadKey(true);
            ResForegroundColor();
        }

        // diskparty (shows disk usages)
        static void DiskParty()
        {
            DriveInfo[] drives = DriveInfo.GetDrives();
            
            foreach (DriveInfo d in drives)
            {
                try
                {
                    if (!d.IsReady)
                        continue;

                    long total = d.TotalSize;
                    long free = d.TotalFreeSpace;
                    long used = total - free;

                    int percent = (int)((used * 100) / total);
                    int bars = percent / 10;

                    Console.ForegroundColor = ConsoleColor.Yellow;
                    Console.Write($"{d.Name} [");

                    for (int i = 0; i < 10; i++)
                    {
                        Console.Write(i < bars ? "#" : "-");
                    }

                    Console.WriteLine($"] {percent}%");
                }
                catch
                {
                    Console.ForegroundColor = ConsoleColor.Red;
                    Console.WriteLine($"failed to read {d.Name}");
                }
            }

            ResForegroundColor();
        }

        // stars ascii :D
        static void Stars()
        {
            Random r = new Random();

            int width = Console.WindowWidth;
            int height = Console.WindowHeight;

            // fade animation
            char[] fadeChars = { '.', '+', '*', '0', '*', '+', '.' };

            Console.CursorVisible = false;

            // store stars + animation frame
            int[,] stars = new int[width, height];

            while (!Console.KeyAvailable)
            {
                Console.SetCursorPosition(0, 0);

                for (int y = 0; y < height - 1; y++)
                {
                    for (int x = 0; x < width; x++)
                    {
                        // randomly spawn star
                        if (stars[x, y] == 0 && r.Next(0, 90) == 0)
                        {
                            stars[x, y] = 1;
                        }

                        int frame = stars[x, y];

                        if (frame > 0)
                        {
                            Console.ForegroundColor = currentFg;
                            Console.BackgroundColor = currentBg;

                            Console.Write(fadeChars[frame - 1]);

                            frame++;

                            // end animation
                            if (frame > fadeChars.Length)
                                frame = 0;

                            stars[x, y] = frame;
                        }
                        else
                        {
                            Console.Write(" ");
                        }
                    }

                    Console.WriteLine();
                }

                Thread.Sleep(80);
            }

            Console.ReadKey(true);

            Console.CursorVisible = true;

            Console.ForegroundColor = currentFg;
            Console.BackgroundColor = currentBg;

            Console.Clear();
        }


        // imports
        // usage: import <whichcommand>

        // which command to import
        static void ImportCmd(string[] args)
        {
            if (args.Length < 2)
            {
                Console.ForegroundColor = ConsoleColor.Cyan;
                Console.WriteLine("usage: importcmd <cmd> (/listimportedcmds)");
                Console.WriteLine("<cmd> - the command needed the import");
                Console.WriteLine("(/listimportedcmds) (or simply enter /list) - shows the list of ALL (yes, ENTIRE) imported commands YOU entered");
                Console.ForegroundColor = ConsoleColor.DarkRed;
                Console.WriteLine("\nWHATEVER YOU DO DONT FCKING PUT 2 SWITCHES AT ONCE OTHERWISE I EXPLOD-");
                Console.ForegroundColor = ConsoleColor.Cyan;
                Console.WriteLine("and to put cherry on top, \"importcmd\" command exists for special commands");

                // list of importable commands:
                Console.ForegroundColor = ConsoleColor.Yellow;
                Console.WriteLine("\nalso here are list of commands that needs to be imported in order to use:");
                Console.WriteLine("- a snake game i made - fissnake / snake");
                Console.WriteLine("- create a .fis file fiscmd script - fisscript / scriptfile / script / scr");
                Console.WriteLine("- draw - fisdraw / fispaint / draw");
                Console.WriteLine("- view the stars :D - fisstars / fisstar / stars / star");
    

                        ResForegroundColor();
                return;
            }

            string cmd = args[1].ToLower();

            // imported command list
            var importList = new[]
            {
            new
              {
                Names = new[] { "fissnake", "snake" },
                Display = "fissnake / snake",
                Imported = importedFissnake,
                ImportAction = new Action(() => importedFissnake = true)
            },
            new
            {
                Names = new[] { "fisscript", "scriptfile", "script", "scr" },
                Display = "fisscript / scriptfile / script / scr",
                Imported = importedFisscript,
                ImportAction = new Action(() => importedFisscript = true)
            },
            new
            {
                Names = new[] { "fisdraw","draw","fispaint" },
                Display = "fisdraw / fispaint / draw",
                Imported = importedFisdraw,
                ImportAction = new Action(() => importedFisdraw = true)
            },
            new
            {
                Names = new[] { "fisstars","fisstar","stars","star" },
                Display = "fisstars / fisstar / stars / star",
                Imported = importedStars,
                ImportAction = new Action(() => importedStars = true)
            }
            // now add any importable commands like this
            /*
            new
            {
                Names = new[] { "alias1", "alias2" },
                Display = "alias1 / alias2",
                Imported = [add a gloabl bool in Main() and put that in here]
                ImportAction = new Action(() => [the same global bool] = true)
            },
            */
            };

            // list imported commands
            if (cmd == "/ls" ||cmd == "/list" || cmd == "/listimportedcmds" || cmd == "/listimportedcommands")
            {
                Console.ForegroundColor = ConsoleColor.Cyan;
                Console.WriteLine("currently imported commands:\n");

                bool anythingImported = false;

                foreach (var item in importList)
                {
                    if (item.Imported)
                    {
                        Console.ForegroundColor = ConsoleColor.Yellow;
                        Console.WriteLine($"- {item.Display}");
                        anythingImported = true;
                    }
                }

                if (!anythingImported)
                {
                    Console.ForegroundColor = ConsoleColor.Red;
                    Console.WriteLine("bro imported absolutely nothing :sob::pray:");
                }

                ResForegroundColor();
                return;
            }

            // import handler
            foreach (var item in importList)
            {
                if (item.Names.Contains(cmd))
                {
                    if (item.Imported)
                    {
                        Console.ForegroundColor = ConsoleColor.Yellow;
                        Console.WriteLine($"{item.Names[0]} already imported bro");
                        ResForegroundColor();
                        return;
                    }

                    item.ImportAction();

                    Console.ForegroundColor = ConsoleColor.Cyan;
                    Console.WriteLine($"successfully imported {item.Names[0]} :D");
                    ResForegroundColor();
                    return;
                }
            }

            // unknown command
            Console.ForegroundColor = ConsoleColor.Red;
            Console.WriteLine($"unknown import command: '{cmd}'");
            ResForegroundColor();
        }

        // unimported command warning
        static bool WarnNotImported(bool whatcmdtobeexact, bool breakorno=true)
        {
            bool yes = false;

            if (!whatcmdtobeexact)
            {
                Console.ForegroundColor = ConsoleColor.Cyan;
                Console.Write("command is valid,");
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine(" but its not imported yet");
                Console.ForegroundColor = ConsoleColor.Yellow;
                Console.WriteLine("use the command \"import\" to import them");
                ResForegroundColor();
                yes = true;
            }

            return yes;
        }

        // fissnake
        static void Snake(string[] args)
        {
            // no switches = usages
            if (args.Length == 1)
            {
                Console.ForegroundColor = ConsoleColor.Cyan;
                Console.WriteLine("usage: fissnake (/default) (/spd [int]) (/size [int])\n");

                Console.WriteLine("(/default) - optional switch used to give out a default size + difficulty");

                Console.Write("(/spd [int]) - set the speed of the game");

                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine(" (do NOT set ts to 10 unless ur secretly a machine)");

                Console.ForegroundColor = ConsoleColor.Cyan;
                Console.WriteLine("(/size [int]) - set the border size");

                Console.ForegroundColor = ConsoleColor.DarkCyan;
                Console.Write("\nusing no switches just opens this usage page cuz im NOT reading minds :skull:\n\nalso in case u dont know, [int] in those 2 switches");
                Console.ForegroundColor = ConsoleColor.Magenta;
                Console.WriteLine(" are basically number needed to input");

                ResForegroundColor();
                return;
            }

            Console.CursorVisible = false;

            int width = 40;
            int height = 20;

            int speed = 5; // default difficulty
            int gameDelay = 90;

            // switches
            for (int i = 1; i < args.Length; i++)
            {
                switch (args[i].ToLower())
                {
                    // fissnake /default
                    case "/default":
                        width = 40;
                        height = 20;
                        speed = 5;
                        break;


                    // fissnake /spd 1-10
                    case "/spd":
                        if (i + 1 < args.Length)
                        {
                            if (int.TryParse(args[i + 1], out int spd))
                            {
                                if (spd < 1 || spd > 10)
                                {
                                    Console.ForegroundColor = ConsoleColor.Red;
                                    Console.WriteLine("speed can only be between 1 - 10 lil bro :sob::wilted-rose:");

                                    ResForegroundColor();
                                    return;
                                }

                                speed = spd;

                                // convert difficulty to actual delay
                                // 1 = slow
                                // 10 = insanity
                                gameDelay = 140 - (spd * 12);
                            }
                            else
                            {
                                Console.ForegroundColor = ConsoleColor.Red;
                                Console.WriteLine($"'{args[i + 1]}' is NOT a valid integer lil bro");

                                ResForegroundColor();
                                return;
                            }
                        }
                        break;

                    // fissnake /size 60
                    case "/size":
                        if (i + 1 < args.Length)
                        {
                            if (int.TryParse(args[i + 1], out int size))
                            {
                                if (size < 10)
                                {
                                    Console.ForegroundColor = ConsoleColor.Red;
                                    Console.WriteLine("too microscopic, cancelled");

                                    ResForegroundColor();
                                    return;
                                } else if (size >= 170)
                                {
                                    Console.ForegroundColor = ConsoleColor.Red;
                                    Console.WriteLine("too gigantic, cancelled");

                                    ResForegroundColor();
                                    return;
                                }

                                width = size;
                                height = size / 2;
                            }
                            else
                            {
                                Console.ForegroundColor = ConsoleColor.Red;
                                Console.WriteLine($"'{args[i + 1]}' is NOT a valid integer bro :sob::wilted-rose:");

                                ResForegroundColor();
                                return;
                            }
                        }
                        break;

                    // help
                    case "/help":
                    case "/?":
                        Console.ForegroundColor = ConsoleColor.Cyan;
                        Console.WriteLine("usage: fissnake (/default) (/spd [int]) (/size [int])");

                        Console.WriteLine("(/default) - optional switch used to give out a default size + difficulty");

                        Console.Write("(/spd [int]) - set the speed of the game");

                        Console.ForegroundColor = ConsoleColor.Red;
                        Console.WriteLine(" (do NOT set ts to 10 unless ur secretly a machine)");

                        Console.ForegroundColor = ConsoleColor.Cyan;
                        Console.WriteLine("(/size [int]) - set the border size");

                        ResForegroundColor();
                        return;
                }
            }

            int score = 0;

            Random rnd = new Random();

            int headX = width / 2;
            int headY = height / 2;

            int foodX = rnd.Next(1, width - 1);
            int foodY = rnd.Next(1, height - 1);

            int velX = 1;
            int velY = 0;

            bool gameOver = false;
            bool paused = false;

            List<(int x, int y)> snake = new List<(int x, int y)>();
            snake.Add((headX, headY));

            while (!gameOver)
            {
                // controls
                if (Console.KeyAvailable)
                {
                    ConsoleKey key = Console.ReadKey(true).Key;

                    switch (key)
                    {
						// move up
                        case ConsoleKey.UpArrow:
                        case ConsoleKey.W:
						case ConsoleKey.I:
                            if (velY != 1)
                            {
                                velX = 0;
                                velY = -1;
                            }
                            break;

						// move down
                        case ConsoleKey.DownArrow:
                        case ConsoleKey.S:
						case ConsoleKey.K:
                            if (velY != -1)
                            {
                                velX = 0;
                                velY = 1;
                            }
                            break;

						// move left
                        case ConsoleKey.LeftArrow:
                        case ConsoleKey.A:
						case ConsoleKey.J:
                            if (velX != 1)
                            {
                                velX = -1;
                                velY = 0;
                            }
                            break;

						// move right
                        case ConsoleKey.RightArrow:
                        case ConsoleKey.D:
						case ConsoleKey.L:
                            if (velX != -1)
                            {
                                velX = 1;
                                velY = 0;
                            }
                            break;

                        // pause
                        case ConsoleKey.Spacebar:
						case ConsoleKey.P:
                            paused = !paused;
                            break;

                        // quit
                        case ConsoleKey.Escape:
						case ConsoleKey.Q:
                            Console.Clear();
                            Console.CursorVisible = true;

                            Console.ForegroundColor = ConsoleColor.Red;
                            Console.WriteLine("bro rage quitted :skull:");

                            ResForegroundColor();
                            return;
                    }
                }

                // pause screen
                if (paused)
                {
                    Console.Clear();

                    Console.ForegroundColor = ConsoleColor.Yellow;
                    Console.WriteLine("fissnake - PAUSED");
                    Console.WriteLine("SPACE / P = resume");
                    Console.WriteLine("ESC / Q = quit\n");

                    Thread.Sleep(50);
                    continue;
                }

                // movement
                headX += velX;
                headY += velY;

                // wall collision
                if (headX <= 0 || headX >= width - 1 ||
                    headY <= 0 || headY >= height - 1)
                {
                    gameOver = true;
                }

                // self collision
                foreach (var part in snake)
                {
                    if (part.x == headX && part.y == headY)
                    {
                        gameOver = true;
                    }
                }

                // add head
                snake.Insert(0, (headX, headY));

                // food collision
                if (headX == foodX && headY == foodY)
                {
                    score++;

                    // prevent food spawning inside snake
                    do
                    {
                        foodX = rnd.Next(1, width - 1);
                        foodY = rnd.Next(1, height - 1);
                    }
                    while (snake.Any(s => s.x == foodX && s.y == foodY));
                }
                else
                {
                    snake.RemoveAt(snake.Count - 1);
                }

                // draw
                Console.Clear();

                Console.ForegroundColor = ConsoleColor.Cyan;
                Console.WriteLine($"fissnake - score: {score}");
                Console.WriteLine($"snake length: {snake.Count}");
                Console.WriteLine($"speed level: {speed}/10");
                Console.WriteLine("SPACE / P = pause | ESC / Q = quit\n");

                for (int y = 0; y < height; y++)
                {
                    for (int x = 0; x < width; x++)
                    {
                        // walls
                        if (x == 0 || x == width - 1 ||
                            y == 0 || y == height - 1)
                        {
                            Console.ForegroundColor = ConsoleColor.DarkGray;
                            Console.Write("#");
                        }

                        // food
                        else if (x == foodX && y == foodY)
                        {
                            Console.ForegroundColor = ConsoleColor.Red;
                            Console.Write("@");
                        }

                        // snake
                        else
                        {
                            bool printed = false;

                            for (int i = 0; i < snake.Count; i++)
                            {
                                if (snake[i].x == x && snake[i].y == y)
                                {
                                    if (i == 0)
                                    {
                                        Console.ForegroundColor = ConsoleColor.Green;
                                        Console.Write("O"); // head
                                    }
                                    else
                                    {
                                        Console.ForegroundColor = ConsoleColor.DarkGreen;
                                        Console.Write("*"); // tail
                                    }

                                    printed = true;
                                    break;
                                }
                            }

							// blank space
                            if (!printed)
                            {
                                Console.Write(" ");
                            }
                        }
                    }

                    Console.WriteLine();
                }

                Thread.Sleep(gameDelay);
            }

            Console.Clear();
            Console.CursorVisible = true;

            Console.ForegroundColor = ConsoleColor.Red;
            TypeWrite("GAME OVER", 50);

            Thread.Sleep(1000);

            Console.ForegroundColor = ConsoleColor.Yellow;
            TypeWrite($"final score: {score}", 25);

            Thread.Sleep(700);

            Console.ForegroundColor = ConsoleColor.Cyan;
            TypeWrite($"final snake length: {snake.Count}", 5);

            Thread.Sleep(1000);
			
			Console.CursorVisible = true;

            ResForegroundColor();
        }

        // fisscript
        static void FisScript(string[] args)
        {
            // remove command name itself
            string[] actualArgs = args.Skip(1).ToArray();

            // /guide mode (no file execution)
            if (actualArgs.Length >= 1 && actualArgs[0].StartsWith("/guide"))
            {
                string guideArg = actualArgs.Length >= 2 ? actualArgs[1].ToLower() : "";

                ShowGuideFisscript(guideArg);
                return;
            }

            // no args after command
            if (actualArgs.Length < 1)
            {
                Console.ForegroundColor = ConsoleColor.Cyan;
                Console.WriteLine("usage: script [file.fis] (/guide)");
                Console.WriteLine("[file.fis] - any files with the .fis file extension");
                Console.WriteLine("(/guide [command]) - optional switch for actually guiding YOU on how to use ts");
                Console.WriteLine("(/guide [command]) - and also [command] is optional, u type the command, it will guide u the same command");

                Console.ForegroundColor = ConsoleColor.DarkCyan;
                Console.WriteLine("\nhow to comment: #");
                Console.WriteLine("example: # this is a comment :D");

                ResForegroundColor();
                return;
            }

            // join remaining args
            string inputPath = string.Join(" ", actualArgs);

            // remove quotes
            inputPath = inputPath.Trim('"');

            string fullPath = "";

            try
            {
                // combine with current fiscmd dir
                fullPath = Path.GetFullPath(Path.Combine(currentDir, inputPath));
            }
            catch
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("invalid path lil bro");
                ResForegroundColor();
                return;
            }

            // exists?
            if (!File.Exists(fullPath))
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("script not found");
                ResForegroundColor();
                return;
            }

            // extension check
            if (Path.GetExtension(fullPath).ToLower() != ".fis")
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("ts is barely even a script");
                ResForegroundColor();
                return;
            }

            // read all lines
            string[] lines = File.ReadAllLines(fullPath);

            // random object
            Random rnd = new Random();

            // runtime storage
            Dictionary<string, string> vars = new Dictionary<string, string>();
            Dictionary<string, List<string>> functions = new Dictionary<string, List<string>>();

            // else statement (last if result)
            bool lastIfResult = false;

            // script start message
            Console.ForegroundColor = ConsoleColor.DarkGray;
            Console.WriteLine($"running script: {Path.GetFileName(fullPath)}\n\nlogs:");
            ResForegroundColor();

            // line runner
            for (int i = 0; i < lines.Length; i++)
            {
                string raw = lines[i];

                string line = raw.Trim();

                // skip empty
                if (line == "")
                    continue;

                // comments
                if (line.StartsWith("#"))
                    continue;

                // split command
                string[] cmd = ParseQuotedArgs(line);

                // no command
                if (cmd.Length < 1)
                    continue;

                // command name
                string command = cmd[0].ToLower();

                // variables
                if (command == "var")
                {
                    // example:
                    // var a = 5
                    // var b = "hello"

                    string content = line.Substring(4).Trim();

                    int eqIndex = content.IndexOf('=');

                    if (eqIndex != -1)
                    {
                        string varName = content.Substring(0, eqIndex).Trim();
                        string value = content.Substring(eqIndex + 1).Trim();

                        // remove quotes
                        if (value.StartsWith("\"") && value.EndsWith("\""))
                            value = value.Substring(1, value.Length - 2);

                        // add/update variable
                        if (vars.ContainsKey(varName))
                            vars[varName] = value;
                        else
                            vars.Add(varName, value);
                    }
                }

                // if statements
                else if (command == "if")
                {
                    bool result = false;

                    int start = line.IndexOf('(');
                    int end = line.LastIndexOf(')');

                    if (start != -1 && end != -1)
                    {
                        string condition = line.Substring(start + 1, end - start - 1);

                        // ==
                        if (condition.Contains("=="))
                        {
                            string[] parts = condition.Split("==");

                            string left = parts[0].Trim();
                            string right = parts[1].Trim();

                            // variable lookup
                            if (vars.ContainsKey(left))
                                left = vars[left];

                            if (vars.ContainsKey(right))
                                right = vars[right];

                            right = right.Trim('"');

                            result = left == right;
                            lastIfResult = result;
                        }

                        // !=
                        else if (condition.Contains("!="))
                        {
                            string[] parts = condition.Split("!=");

                            string left = parts[0].Trim();
                            string right = parts[1].Trim();

                            // variable lookup
                            if (vars.ContainsKey(left))
                                left = vars[left];

                            if (vars.ContainsKey(right))
                                right = vars[right];

                            right = right.Trim('"');

                            result = left != right;
                            lastIfResult = result;
                        }
                    }

                    // true
                    if (result)
                    {
                        i++;

                        int braceLevel = 1;

                        while (i < lines.Length)
                        {
                            string inner = lines[i].Trim();

                            if (inner == "{")
                            {
                                braceLevel++;
                                i++;
                                continue;
                            }

                            if (inner == "}")
                            {
                                braceLevel--;

                                if (braceLevel == 0)
                                    break;

                                i++;
                                continue;
                            }

                            // execute commands here

                            string[] innerCmd = ParseQuotedArgs(inner);

                            if (innerCmd.Length < 1)
                            {
                                i++;
                                continue;
                            }

                            string innerCommand = innerCmd[0].ToLower();

                            // nested if
                            if (innerCommand == "if")
                            {
                                bool innerResult = false;

                                int start2 = inner.IndexOf('(');
                                int end2 = inner.LastIndexOf(')');

                                if (start2 != -1 && end2 != -1)
                                {
                                    string condition = inner.Substring(start2 + 1, end2 - start2 - 1);

                                    // ==
                                    if (condition.Contains("=="))
                                    {
                                        string[] parts = condition.Split(new string[] { "==" }, StringSplitOptions.None);

                                        string left = parts[0].Trim();
                                        string right = parts[1].Trim();

                                        if (vars.ContainsKey(left))
                                            left = vars[left];

                                        if (vars.ContainsKey(right))
                                            right = vars[right];

                                        right = right.Trim('"');

                                        innerResult = left == right;
                                    }

                                    // !=
                                    else if (condition.Contains("!="))
                                    {
                                        string[] parts = condition.Split(new string[] { "!=" }, StringSplitOptions.None);

                                        string left = parts[0].Trim();
                                        string right = parts[1].Trim();

                                        if (vars.ContainsKey(left))
                                            left = vars[left];

                                        if (vars.ContainsKey(right))
                                            right = vars[right];

                                        right = right.Trim('"');

                                        innerResult = left != right;
                                    }
                                }

                                // skip nested block if false
                                if (!innerResult)
                                {
                                    int nestedBrace = 0;

                                    while (i < lines.Length)
                                    {
                                        string skipLine = lines[i].Trim();

                                        if (skipLine == "{")
                                            nestedBrace++;

                                        if (skipLine == "}")
                                        {
                                            nestedBrace--;

                                            if (nestedBrace <= 0)
                                                break;
                                        }

                                        i++;
                                    }
                                }
                            }

                            // variable reassignment
                            else if (inner.Contains("=") && !inner.StartsWith("if"))
                            {
                                int eqIndex2 = inner.IndexOf('=');

                                if (eqIndex2 != -1)
                                {
                                    string varName = inner.Substring(0, eqIndex2).Trim();
                                    string value = inner.Substring(eqIndex2 + 1).Trim();

                                    if (vars.ContainsKey(value))
                                        value = vars[value];

                                    // remove quotes
                                    if (value.StartsWith("\"") && value.EndsWith("\""))
                                        value = value.Substring(1, value.Length - 2);

                                    if (vars.ContainsKey(varName))
                                    {
                                        vars[varName] = value;
                                    }
                                }
                            }

                            // ++
                            else if (inner.EndsWith("++"))
                            {
                                string varName = inner.Replace("++", "").Trim();

                                if (vars.ContainsKey(varName))
                                {
                                    int num;

                                    if (int.TryParse(vars[varName], out num))
                                    {
                                        num++;

                                        vars[varName] = num.ToString();
                                    }
                                }
                            }

                            // print
                            else if (innerCommand == "print")
                            {
                                if (inner.Length > 6)
                                {
                                    string output = inner.Substring(6);

                                    output = ParseVars(output, vars);

                                    Console.WriteLine(output);
                                }
                            }

                            // echo
                            else if (innerCommand == "echo")
                            {
                                if (inner.Length > 5)
                                {
                                    Console.WriteLine(inner.Substring(5));
                                }
                            }

                            // type
                            else if (innerCommand == "type")
                            {
                                if (inner.Length > 5)
                                {
                                    string text = inner.Substring(5);

                                    foreach (char c in text)
                                    {
                                        Console.Write(c);
                                        Thread.Sleep(25);
                                    }

                                    Console.WriteLine();
                                }
                            }

                            // wait
                            else if (innerCommand == "wait" || innerCommand == "sleep")
                            {
                                if (innerCmd.Length >= 2)
                                {
                                    int ms;

                                    if (int.TryParse(innerCmd[1], out ms))
                                        Thread.Sleep(ms);
                                }
                            }

                            // clear
                            else if (innerCommand == "clear" || innerCommand == "cls")
                            {
                                Console.Clear();
                            }

                            // beep
                            else if (innerCommand == "beep")
                            {
                                Console.Beep();
                            }

                            // pause
                            else if (innerCommand == "pause")
                            {
                                Console.ForegroundColor = ConsoleColor.DarkGray;
                                Console.Write("press any key to continue . . . ");

                                ResForegroundColor();

                                Console.ReadKey(true);
                                Console.WriteLine();
                            }

                            i++;
                        }
                    }

                    // false
                    else
                    {
                        while (i < lines.Length)
                        {
                            if (lines[i].Trim() == "}")
                                break;

                            i++;
                        }
                    }
                }

                // else statements
                else if (command == "else")
                {
                    // skip else if previous if was true
                    if (lastIfResult)
                    {
                        while (i < lines.Length)
                        {
                            if (lines[i].Trim() == "}")
                                break;

                            i++;
                        }
                    }

                    // run else block
                    else
                    {
                        i++;

                        int braceLevel = 1;

                        while (i < lines.Length)
                        {
                            string inner = lines[i].Trim();

                            if (inner == "{")
                            {
                                braceLevel++;
                                i++;
                                continue;
                            }

                            if (inner == "}")
                            {
                                braceLevel--;

                                if (braceLevel == 0)
                                    break;

                                i++;
                                continue;
                            }

                            // execute commands here

                            string[] innerCmd = ParseQuotedArgs(inner);

                            if (innerCmd.Length < 1)
                            {
                                i++;
                                continue;
                            }

                            string innerCommand = innerCmd[0].ToLower();

                            // nested if
                            if (innerCommand == "if")
                            {
                                bool innerResult = false;

                                int start2 = inner.IndexOf('(');
                                int end2 = inner.LastIndexOf(')');

                                if (start2 != -1 && end2 != -1)
                                {
                                    string condition = inner.Substring(start2 + 1, end2 - start2 - 1);

                                    // ==
                                    if (condition.Contains("=="))
                                    {
                                        string[] parts = condition.Split(new string[] { "==" }, StringSplitOptions.None);

                                        string left = parts[0].Trim();
                                        string right = parts[1].Trim();

                                        if (vars.ContainsKey(left))
                                            left = vars[left];

                                        if (vars.ContainsKey(right))
                                            right = vars[right];

                                        right = right.Trim('"');

                                        innerResult = left == right;
                                    }

                                    // !=
                                    else if (condition.Contains("!="))
                                    {
                                        string[] parts = condition.Split(new string[] { "!=" }, StringSplitOptions.None);

                                        string left = parts[0].Trim();
                                        string right = parts[1].Trim();

                                        if (vars.ContainsKey(left))
                                            left = vars[left];

                                        if (vars.ContainsKey(right))
                                            right = vars[right];

                                        right = right.Trim('"');

                                        innerResult = left != right;
                                    }
                                }

                                // skip nested block if false
                                if (!innerResult)
                                {
                                    int nestedBrace = 0;

                                    while (i < lines.Length)
                                    {
                                        string skipLine = lines[i].Trim();

                                        if (skipLine == "{")
                                            nestedBrace++;

                                        if (skipLine == "}")
                                        {
                                            nestedBrace--;

                                            if (nestedBrace <= 0)
                                                break;
                                        }

                                        i++;
                                    }
                                }
                            }

                            // variable reassignment
                            else if (inner.Contains("=") && !inner.StartsWith("if"))
                            {
                                int eqIndex2 = inner.IndexOf('=');

                                if (eqIndex2 != -1)
                                {
                                    string varName = inner.Substring(0, eqIndex2).Trim();
                                    string value = inner.Substring(eqIndex2 + 1).Trim();

                                    if (vars.ContainsKey(value))
                                        value = vars[value];

                                    // remove quotes
                                    if (value.StartsWith("\"") && value.EndsWith("\""))
                                        value = value.Substring(1, value.Length - 2);

                                    if (vars.ContainsKey(varName))
                                    {
                                        vars[varName] = value;
                                    }
                                }
                            }

                            // ++
                            else if (inner.EndsWith("++"))
                            {
                                string varName = inner.Replace("++", "").Trim();

                                if (vars.ContainsKey(varName))
                                {
                                    int num;

                                    if (int.TryParse(vars[varName], out num))
                                    {
                                        num++;

                                        vars[varName] = num.ToString();
                                    }
                                }
                            }

                            // print
                            else if (innerCommand == "print")
                            {
                                if (inner.Length > 6)
                                {
                                    string output = inner.Substring(6);

                                    output = ParseVars(output, vars);

                                    Console.WriteLine(output);
                                }
                            }

                            // echo
                            else if (innerCommand == "echo")
                            {
                                if (inner.Length > 5)
                                {
                                    Console.WriteLine(inner.Substring(5));
                                }
                            }

                            // type
                            else if (innerCommand == "type")
                            {
                                if (inner.Length > 5)
                                {
                                    string text = inner.Substring(5);

                                    foreach (char c in text)
                                    {
                                        Console.Write(c);
                                        Thread.Sleep(25);
                                    }

                                    Console.WriteLine();
                                }
                            }

                            // wait
                            else if (innerCommand == "wait" || innerCommand == "sleep")
                            {
                                if (innerCmd.Length >= 2)
                                {
                                    int ms;

                                    if (int.TryParse(innerCmd[1], out ms))
                                        Thread.Sleep(ms);
                                }
                            }

                            // clear
                            else if (innerCommand == "clear" || innerCommand == "cls")
                            {
                                Console.Clear();
                            }

                            // beep
                            else if (innerCommand == "beep")
                            {
                                Console.Beep();
                            }

                            // pause
                            else if (innerCommand == "pause")
                            {
                                Console.ForegroundColor = ConsoleColor.DarkGray;
                                Console.Write("press any key to continue . . . ");

                                ResForegroundColor();

                                Console.ReadKey(true);
                                Console.WriteLine();
                            }

                            i++;
                        }
                    }
                }

                // ++
                else if (line.EndsWith("++"))
                {
                    string varName = line.Replace("++", "").Trim();

                    if (vars.ContainsKey(varName))
                    {
                        int num;

                        if (int.TryParse(vars[varName], out num))
                        {
                            num++;

                            vars[varName] = num.ToString();
                        }
                    }
                }

                // variable reassignment
                else if (line.Contains("=") && !line.StartsWith("if"))
                {
                    int eqIndex = line.IndexOf('=');

                    if (eqIndex != -1)
                    {
                        string varName = line.Substring(0, eqIndex).Trim();
                        string value = line.Substring(eqIndex + 1).Trim();

                        // remove quotes
                        if (value.StartsWith("\"") && value.EndsWith("\""))
                            value = value.Substring(1, value.Length - 2);

                        // update variable
                        if (vars.ContainsKey(varName))
                        {
                            vars[varName] = value;
                        }
                    }
                }

                // print
                else if (command == "print")
                {
                    if (line.Length > 6)
                    {
                        string output = line.Substring(6);

                        output = ParseVars(output, vars);

                        Console.WriteLine(output);
                    }
                }

                // echo
                else if (command == "echo")
                {
                    if (line.Length > 5)
                    {
                        Console.WriteLine(line.Substring(5));
                    }
                }

                // typewriter
                else if (command == "type")
                {
                    if (line.Length > 5)
                    {
                        string text = line.Substring(5);

                        foreach (char c in text)
                        {
                            Console.Write(c);
                            Thread.Sleep(25);
                        }

                        Console.WriteLine();
                    }
                }

                // wait
                else if (command == "wait" || command == "sleep")
                {
                    if (cmd.Length >= 2)
                    {
                        int ms;

                        if (int.TryParse(cmd[1], out ms))
                            Thread.Sleep(ms);
                    }
                }

                // clear
                else if (command == "clear" || command == "cls")
                {
                    Console.Clear();
                }

                // title
                else if (command == "title")
                {
                    if (line.Length > 6)
                        Console.Title = line.Substring(6);
                }

                // random
                else if (command == "random")
                {
                    if (cmd.Length >= 3)
                    {
                        int min;
                        int max;

                        if (int.TryParse(cmd[1], out min) &&
                            int.TryParse(cmd[2], out max))
                        {
                            Console.WriteLine(rnd.Next(min, max + 1));
                        }
                    }
                }

                // functions
                else if (command == "func")
                {
                    // example:
                    // func hello

                    if (cmd.Length >= 2)
                    {
                        string funcName = cmd[1];

                        List<string> funcLines = new List<string>();

                        i++;

                        int braceLevel = 0;

                        while (i < lines.Length)
                        {
                            string funcLine = lines[i].Trim();

                            // opening brace
                            if (funcLine == "{")
                            {
                                braceLevel++;

                                funcLines.Add(funcLine);

                                i++;
                                continue;
                            }

                            // closing brace
                            if (funcLine == "}")
                            {
                                braceLevel--;

                                // ONLY add nested braces
                                if (braceLevel > 0)
                                    funcLines.Add(funcLine);

                                // end function completely
                                if (braceLevel <= 0)
                                    break;

                                i++;
                                continue;
                            }

                            funcLines.Add(funcLine);

                            i++;
                        }

                        // add/update function
                        if (functions.ContainsKey(funcName))
                            functions[funcName] = funcLines;
                        else
                            functions.Add(funcName, funcLines);
                    }
                }

                // call function
                else if (command == "call")
                {
                    // example:
                    // call hello

                    if (cmd.Length >= 2)
                    {
                        string funcName = cmd[1];

                        // exists?
                        if (functions.ContainsKey(funcName))
                        {
                            List<string> funcLines = functions[funcName];

                            for (int fi = 0; fi < funcLines.Count; fi++)
                            {
                                string funcLine = funcLines[fi].Trim();

                                // skip empty
                                if (funcLine == "")
                                    continue;

                                // comments
                                if (funcLine.StartsWith("#"))
                                    continue;

                                // parse command
                                string[] funcCmd = ParseQuotedArgs(funcLine);

                                if (funcCmd.Length < 1)
                                    continue;

                                string funcCommand = funcCmd[0].ToLower();

                                // skip braces
                                if (funcLine == "{" || funcLine == "}")
                                    continue;

                                // if inside function
                                if (funcCommand == "if")
                                {
                                    bool result = false;

                                    int start = funcLine.IndexOf('(');
                                    int end = funcLine.LastIndexOf(')');

                                    if (start != -1 && end != -1)
                                    {
                                        string condition = funcLine.Substring(start + 1, end - start - 1);

                                        // ==
                                        if (condition.Contains("=="))
                                        {
                                            string[] parts = condition.Split(new string[] { "==" }, StringSplitOptions.None);

                                            string left = parts[0].Trim();
                                            string right = parts[1].Trim();

                                            if (vars.ContainsKey(left))
                                                left = vars[left];

                                            if (vars.ContainsKey(right))
                                                right = vars[right];

                                            right = right.Trim('"');

                                            result = left == right;
                                            lastIfResult = result;
                                        }

                                        // !=
                                        else if (condition.Contains("!="))
                                        {
                                            string[] parts = condition.Split(new string[] { "!=" }, StringSplitOptions.None);

                                            string left = parts[0].Trim();
                                            string right = parts[1].Trim();

                                            if (vars.ContainsKey(left))
                                                left = vars[left];

                                            if (vars.ContainsKey(right))
                                                right = vars[right];

                                            right = right.Trim('"');

                                            result = left != right;
                                            lastIfResult = result;
                                        }
                                    }

                                    // if false, skip block
                                    if (!result)
                                    {
                                        while (fi < funcLines.Count)
                                        {
                                            if (funcLines[fi].Trim() == "}")
                                                break;

                                            fi++;
                                        }
                                    }

                                    continue;
                                }

                                // ++
                                else if (funcLine.EndsWith("++"))
                                {
                                    string varName = funcLine.Replace("++", "").Trim();

                                    if (vars.ContainsKey(varName))
                                    {
                                        int num;

                                        if (int.TryParse(vars[varName], out num))
                                        {
                                            num++;

                                            vars[varName] = num.ToString();
                                        }
                                    }
                                }

                                // +=
                                else if (funcLine.Contains("+="))
                                {
                                    string[] parts = funcLine.Split(new string[] { "+=" }, StringSplitOptions.None);

                                    if (parts.Length >= 2)
                                    {
                                        string varName = parts[0].Trim();
                                        string addValue = parts[1].Trim();

                                        if (vars.ContainsKey(addValue))
                                            addValue = vars[addValue];

                                        if (vars.ContainsKey(varName))
                                        {
                                            int left;
                                            int right;

                                            if (int.TryParse(vars[varName], out left) &&
                                                int.TryParse(addValue, out right))
                                            {
                                                vars[varName] = (left + right).ToString();
                                            }
                                        }
                                    }
                                }

                                // a = a+1
                                else if (funcLine.Contains("=") && funcLine.Contains("+"))
                                {
                                    int eqIndex = funcLine.IndexOf('=');

                                    string varName = funcLine.Substring(0, eqIndex).Trim();
                                    string expression = funcLine.Substring(eqIndex + 1).Trim();

                                    string[] math = expression.Split('+');

                                    if (math.Length >= 2)
                                    {
                                        string leftSide = math[0].Trim();
                                        string rightSide = math[1].Trim();

                                        if (vars.ContainsKey(leftSide))
                                            leftSide = vars[leftSide];

                                        if (vars.ContainsKey(rightSide))
                                            rightSide = vars[rightSide];

                                        int left;
                                        int right;

                                        if (int.TryParse(leftSide, out left) &&
                                            int.TryParse(rightSide, out right))
                                        {
                                            vars[varName] = (left + right).ToString();
                                        }
                                    }
                                }

                                // variable reassignment
                                else if (funcLine.Contains("=") && !funcLine.StartsWith("if"))
                                {
                                    int eqIndex = funcLine.IndexOf('=');

                                    if (eqIndex != -1)
                                    {
                                        string varName = funcLine.Substring(0, eqIndex).Trim();
                                        string value = funcLine.Substring(eqIndex + 1).Trim();

                                        if (vars.ContainsKey(value))
                                            value = vars[value];

                                        // remove quotes
                                        if (value.StartsWith("\"") && value.EndsWith("\""))
                                            value = value.Substring(1, value.Length - 2);

                                        // update variable
                                        if (vars.ContainsKey(varName))
                                        {
                                            vars[varName] = value;
                                        }
                                    }
                                }

                                // print
                                else if (funcCommand == "print")
                                {
                                    if (funcLine.Length > 6)
                                    {
                                        string output = funcLine.Substring(6);

                                        output = ParseVars(output, vars);

                                        Console.WriteLine(output);
                                    }
                                }

                                // echo
                                else if (funcCommand == "echo")
                                {
                                    if (funcLine.Length > 5)
                                    {
                                        Console.WriteLine(funcLine.Substring(5));
                                    }
                                }

                                // type
                                else if (funcCommand == "type")
                                {
                                    if (funcLine.Length > 5)
                                    {
                                        string text = funcLine.Substring(5);

                                        foreach (char c in text)
                                        {
                                            Console.Write(c);
                                            Thread.Sleep(25);
                                        }

                                        Console.WriteLine();
                                    }
                                }

                                // wait
                                else if (funcCommand == "wait" || funcCommand == "sleep")
                                {
                                    if (funcCmd.Length >= 2)
                                    {
                                        int ms;

                                        if (int.TryParse(funcCmd[1], out ms))
                                            Thread.Sleep(ms);
                                    }
                                }

                                // clear
                                else if (funcCommand == "clear" || funcCommand == "cls")
                                {
                                    Console.Clear();
                                }

                                // beep
                                else if (funcCommand == "beep")
                                {
                                    Console.Beep();
                                }

                                // pause
                                else if (funcCommand == "pause")
                                {
                                    Console.ForegroundColor = ConsoleColor.DarkGray;
                                    Console.Write("press any key to continue . . . ");

                                    ResForegroundColor();

                                    Console.ReadKey(true);
                                    Console.WriteLine();
                                }

                                // unknown
                                else
                                {
                                    Console.ForegroundColor = ConsoleColor.DarkRed;
                                    Console.WriteLine($"unknown command in function: {funcCmd[0]}");
                                    ResForegroundColor();
                                }
                            }
                        }

                        // not found
                        else
                        {
                            Console.ForegroundColor = ConsoleColor.DarkRed;
                            Console.WriteLine($"function not found: {funcName}");
                            ResForegroundColor();
                        }
                    }
                }

                // color
                else if (command == "color")
                {
                    if (cmd.Length >= 2)
                    {
                        switch (cmd[1].ToLower())
                        {
                            case "black":
                                Console.ForegroundColor = ConsoleColor.Black;
                                break;

                            case "blue":
                                Console.ForegroundColor = ConsoleColor.Blue;
                                break;

                            case "cyan":
                                Console.ForegroundColor = ConsoleColor.Cyan;
                                break;

                            case "darkblue":
                                Console.ForegroundColor = ConsoleColor.DarkBlue;
                                break;

                            case "darkcyan":
                                Console.ForegroundColor = ConsoleColor.DarkCyan;
                                break;

                            case "darkgray":
                                Console.ForegroundColor = ConsoleColor.DarkGray;
                                break;

                            case "darkgreen":
                                Console.ForegroundColor = ConsoleColor.DarkGreen;
                                break;

                            case "darkmagenta":
                                Console.ForegroundColor = ConsoleColor.DarkMagenta;
                                break;

                            case "darkred":
                                Console.ForegroundColor = ConsoleColor.DarkRed;
                                break;

                            case "darkyellow":
                                Console.ForegroundColor = ConsoleColor.DarkYellow;
                                break;

                            case "gray":
                                Console.ForegroundColor = ConsoleColor.Gray;
                                break;

                            case "green":
                                Console.ForegroundColor = ConsoleColor.Green;
                                break;

                            case "magenta":
                                Console.ForegroundColor = ConsoleColor.Magenta;
                                break;

                            case "red":
                                Console.ForegroundColor = ConsoleColor.Red;
                                break;

                            case "white":
                                Console.ForegroundColor = ConsoleColor.White;
                                break;

                            case "yellow":
                                Console.ForegroundColor = ConsoleColor.Yellow;
                                break;

                            case "reset":
                                ResForegroundColor();
                                break;
                        }
                    }
                }

                // beep
                else if (command == "beep")
                {
                    Console.Beep();
                }

                // pause
                else if (command == "pause")
                {
                    Console.ForegroundColor = ConsoleColor.DarkGray;
                    Console.Write("press any key to continue . . . ");

                    ResForegroundColor();

                    Console.ReadKey(true);
                    Console.WriteLine();
                }

                // pause without prompt
                else if (command == "paunul")
                {
                    Console.ReadKey(true);
                }

                // unknown command
                else
                {
                    Console.ForegroundColor = ConsoleColor.DarkRed;
                    Console.WriteLine($"unknown command: {cmd[0]}");
                    ResForegroundColor();
                }
            }

            Console.ForegroundColor = ConsoleColor.DarkGray;
            Console.WriteLine("script finished :D");

            ResForegroundColor();
        }

        // show guide for the Fisscript() void
        static void ShowGuideFisscript(string arg)
        {
            void prnt(string what, bool newline=true)
            {
                if (newline)
                {
                    Console.WriteLine(what);
                } else
                {
                    Console.Write(what);
                }
            }

            void setcol(ConsoleColor col)
            {
                Console.ForegroundColor = col;
            }

            Console.ForegroundColor = ConsoleColor.Cyan;

            if (arg == "" || arg == null)
            {
                setcol(ConsoleColor.Cyan);
                prnt("list of commands:");
                setcol(ConsoleColor.DarkCyan);
                prnt("\nSIMPLE COMMANDS:");
                setcol(ConsoleColor.Yellow);
                prnt("- echo: echo anything after it, simple");
                prnt("- print: same as echo except it prints the variable whenever there are brackets {} that surrounds it");
                prnt("- type: same as echo except it types anything after it, also simple");
                prnt("- pause: \"press any key to continue\"");
                prnt("- paunul: same as pause except it doesnt show the prompt");
                prnt("- color: set a color, thats it");
                prnt("- wait / sleep: waits for how many milleseconds");
                setcol(ConsoleColor.Red);
                prnt("\nADVANCED COMMANDS:");
                setcol(ConsoleColor.Yellow);
                prnt("- if - then statement: if { }");
                prnt(@"example of that:
if (a == 5) {
    a = 6
}");
                prnt("unfortunately no \"else\" :c");
                prnt("\n- var: create a variable\nexample of that: var a = 5");
                prnt("\n- func: make a new function");
                prnt(@"example of that:
var a = 5
func hi {
    print {a}
}");
                prnt("and call that later with \"call\"");
                prnt(@"
call hi
                
(echoes out ""5""");
                setcol(ConsoleColor.DarkYellow);
                prnt("\nGOOFY COMMANDS:");
                setcol(ConsoleColor.Yellow);
                prnt("- beep: just... beeps :skull:");
                prnt("- random: random a number ranged from 1 - 100");
                prnt("\nbasically thats all");
                prnt("# to comment (ex: # this is a comment)");
            }
            else if (arg == "vars" || arg == "var")
            {
                setcol(ConsoleColor.Yellow);
                prnt("var: create a variable\nexample of that: var a = 5");
            }
            else if (arg == "print")
            {
                setcol(ConsoleColor.Yellow);
                prnt("- print: same as echo except it prints the variable whenever there are brackets {} that surrounds it");
            }
            else if (arg == "echo")
            {
                setcol(ConsoleColor.Yellow);
                prnt("- echo: echo anything after it, simple");
            }
            else if (arg == "type")
            {
                setcol(ConsoleColor.Yellow);
                prnt("- type: same as echo except it types anything after it, also simple");
            }
            else if (arg == "if")
            {
                setcol(ConsoleColor.Yellow);
                prnt("- if - then statement: if { }");
                prnt(@"example of that:
if (a == 5) {
    a = 6
}");
                prnt("unfortunately no \"else\" :c");
            }
            else if (arg == "func" || arg == "call")
            {
                setcol(ConsoleColor.Yellow);
                prnt("\n- func: make a new function");
                prnt(@"example of that:
var a = 5
func hi {
    print {a}
}");
                prnt("and call that later with \"call\"");
                prnt(@"
                call hi
                
                (echoes out ""5""");
            }
            else if (arg == "pause")
            {
                setcol(ConsoleColor.Yellow);
                prnt("- pause: \"press any key to continue\"");
            }
            else if (arg == "paunul")
            {
                setcol(ConsoleColor.Yellow);
                prnt("- paunul: same as pause except it doesnt show the prompt");
            }
            else if (arg == "color")
            {
                // white > black
                setcol(ConsoleColor.White);
                prnt("white, ", false);
                setcol(ConsoleColor.Gray);
                prnt("gray, ", false);
                setcol(ConsoleColor.DarkGray);
                prnt("darkgray, ", false);
                setcol(ConsoleColor.Black);
                Console.BackgroundColor = ConsoleColor.White;
                prnt("black", false);
                Console.BackgroundColor = currentBg;
                ResForegroundColor();
                prnt(", ", false);

                // red
                setcol(ConsoleColor.Red);
                prnt("red, ", false);
                setcol(ConsoleColor.DarkRed);
                prnt("darkred, ", false);
                setcol(ConsoleColor.Magenta);
                
                // magenta
                prnt("magenta, ", false);
                setcol(ConsoleColor.DarkMagenta);
                prnt("darkmagenta, ", false);
                
                // blue
                setcol(ConsoleColor.Cyan);
                prnt("cyan, ", false);
                setcol(ConsoleColor.DarkCyan);
                prnt("darkcyan, ", false);
                setcol(ConsoleColor.Blue);
                prnt("blue, ", false);
                setcol(ConsoleColor.DarkBlue);
                prnt("darkblue, ", false);

                // green
                setcol(ConsoleColor.Green);
                prnt("green, ", false);
                setcol(ConsoleColor.DarkGreen);
                prnt("darkgreen, ", false);

                // yellow
                setcol(ConsoleColor.Yellow);
                prnt("yellow, ", false);
                setcol(ConsoleColor.DarkYellow);
                prnt("darkyellow");

                // reset color
                setcol(ConsoleColor.Yellow);
                prnt("\nto reset color, use \"reset\" and thats it");
            }
            else if (arg == "beep")
            {
                setcol(ConsoleColor.Yellow);
                prnt("- beep: just... beeps :skull:");
            }
            else if (arg == "random")
            {
                setcol(ConsoleColor.Yellow);
                prnt("- random: random a number ranged from 1 - 100");
            }
            else if (arg == "sleep" || arg == "wait")
            {
                setcol(ConsoleColor.Yellow);
                prnt("- wait / sleep: waits for how many milleseconds");
            }
            else
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("unknown guide topic :c");
            }

            ResForegroundColor();
        }

        // fisdraw
        static void FisDraw(string[] args)
        {
            Console.Clear();
            Console.CursorVisible = false;

            int width = 60;
            int height = 20;

            char[,] canvas = new char[height, width];

            // fill canvas
            for (int y = 0; y < height; y++)
            {
                for (int x = 0; x < width; x++)
                {
                    canvas[y, x] = ' ';
                }
            }

            int px = width / 2;
            int py = height / 2;

            // current opened file
            string currentFile = null;

            // auto-open file from args
            if (args.Length > 1)
            {
                currentFile =
                    Path.Combine(currentDir,
                    string.Join(" ", args.Skip(1)));

                if (!currentFile.EndsWith(".txt"))
                    currentFile += ".txt";

                if (File.Exists(currentFile))
                {
                    string[] lines = File.ReadAllLines(currentFile);

                    for (int y = 0;
                        y < height && y < lines.Length;
                        y++)
                    {
                        for (int x = 0;
                            x < width &&
                            x < lines[y].Length;
                            x++)
                        {
                            canvas[y, x] = lines[y][x];
                        }
                    }
                }
            }

            // undo / redo stacks
            Stack<char[,]> undoStack = new Stack<char[,]>();
            Stack<char[,]> redoStack = new Stack<char[,]>();

            // clone canvas helper
            char[,] CloneCanvas(char[,] source)
            {
                char[,] copy = new char[height, width];

                for (int y = 0; y < height; y++)
                {
                    for (int x = 0; x < width; x++)
                    {
                        copy[y, x] = source[y, x];
                    }
                }

                return copy;
            }

            // save state before edit
            void SaveState()
            {
                undoStack.Push(CloneCanvas(canvas));

                // new edit clears redo history
                redoStack.Clear();
            }

            bool running = true;

            while (running)
            {
                // draw everything
                Console.SetCursorPosition(0, 0);

                Console.ForegroundColor = ConsoleColor.Cyan;
                Console.WriteLine("fisdraw");

                Console.ForegroundColor = ConsoleColor.DarkGray;

                if (currentFile != null)
                {
                    Console.WriteLine(
                        $"opened: {Path.GetFileName(currentFile)}");
                }
                else
                {
                    Console.WriteLine("opened: untitled");
                }

                Console.WriteLine(@"WASD / IJKL / arrow keys = move | SPACE / ENTER = draw | P = erase
Z = undo | Y = redo | O = save | U = load | C = clear | ESC / Q = exit");
                Console.WriteLine(); // extra line

                for (int y = 0; y < height; y++)
                {
                    for (int x = 0; x < width; x++)
                    {
                        // cursor/player
                        if (x == px && y == py)
                        {
                            Console.ForegroundColor = ConsoleColor.Yellow;
                            Console.Write('@');
                        }
                        else
                        {
                            Console.ForegroundColor = ConsoleColor.White;
                            Console.Write(canvas[y, x]);
                        }
                    }

                    Console.WriteLine();
                }

                // input
                ConsoleKeyInfo key = Console.ReadKey(true);

                switch (key.Key)
                {
                    // up
                    case ConsoleKey.W: // classic WASD
                    case ConsoleKey.I: // right handed IJKL
                    case ConsoleKey.UpArrow: // arrow keys

                        if (py > 0)
                            py--;

                        break;

                    // down
                    case ConsoleKey.S:
                    case ConsoleKey.K:
                    case ConsoleKey.DownArrow:

                        if (py < height - 1)
                            py++;

                        break;

                    // left
                    case ConsoleKey.A:
                    case ConsoleKey.J:
                    case ConsoleKey.LeftArrow:

                        if (px > 0)
                            px--;

                        break;

                    // right
                    case ConsoleKey.D:
                    case ConsoleKey.L:
                    case ConsoleKey.RightArrow:

                        if (px < width - 1)
                            px++;

                        break;

                    // draw
                    case ConsoleKey.Spacebar:
                    case ConsoleKey.Enter:

                        SaveState();

                        canvas[py, px] = '#';

                        break;

                    // erase
                    case ConsoleKey.P:

                        SaveState();

                        canvas[py, px] = ' ';

                        break;

                    // undo
                    case ConsoleKey.Z:

                        if (undoStack.Count > 0)
                        {
                            redoStack.Push(CloneCanvas(canvas));

                            canvas = undoStack.Pop();
                        }

                        break;

                    // redo
                    case ConsoleKey.Y:

                        if (redoStack.Count > 0)
                        {
                            undoStack.Push(CloneCanvas(canvas));

                            canvas = redoStack.Pop();
                        }

                        break;

                    // clear
                    case ConsoleKey.C:

                        SaveState();

                        for (int y = 0; y < height; y++)
                        {
                            for (int x = 0; x < width; x++)
                            {
                                canvas[y, x] = ' ';
                            }
                        }

                        break;

                    // save
                    case ConsoleKey.O:

                        try
                        {
                            // ask filename if none opened
                            if (currentFile == null)
                            {
                                Console.CursorVisible = true;

                                int inputY = height + 5;

                                Console.SetCursorPosition(0, inputY);
                                Console.Write(
                                    new string(' ', Console.WindowWidth));

                                Console.SetCursorPosition(0, inputY);

                                Console.ForegroundColor =
                                    ConsoleColor.Cyan;

                                Console.Write("save as: ");

                                string saveInput = Console.ReadLine();
                                saveInput = saveInput.Trim('"');

                                if (!string.IsNullOrWhiteSpace(saveInput))
                                {
                                    saveInput = saveInput.Trim('"');

                                    if (Path.IsPathRooted(saveInput))
                                    {
                                        currentFile = saveInput;
                                    }
                                    else
                                    {
                                        currentFile =
                                            Path.Combine(currentDir,
                                            saveInput);
                                    }

                                    if (!currentFile.EndsWith(".txt"))
                                        currentFile += ".txt";
                                }

                                Console.CursorVisible = false;
                            }

                            // save file
                            if (currentFile != null)
                            {
                                using (StreamWriter sw =
                                    new StreamWriter(currentFile))
                                {
                                    for (int y = 0; y < height; y++)
                                    {
                                        string line = "";

                                        for (int x = 0; x < width; x++)
                                        {
                                            line += canvas[y, x];
                                        }

                                        sw.WriteLine(line);
                                    }
                                }
                            }

                            Console.Clear();
                        }
                        catch
                        {
                            Console.CursorVisible = false;
                            Console.Clear();
                        }

                        break;

                    // load
                    case ConsoleKey.U:

                        try
                        {
                            Console.CursorVisible = true;

                            int inputY = height + 5;

                            Console.SetCursorPosition(0, inputY);
                            Console.Write(
                                new string(' ', Console.WindowWidth));

                            Console.SetCursorPosition(0, inputY);

                            Console.ForegroundColor =
                                ConsoleColor.Cyan;

                            Console.Write("load file: ");

                            string loadInput = Console.ReadLine();
                            loadInput = loadInput.Trim('"');

                            if (!string.IsNullOrWhiteSpace(loadInput))
                            {
                                string loadPath;

                                if (Path.IsPathRooted(loadInput))
                                {
                                    loadPath = loadInput;
                                }
                                else
                                {
                                    loadPath = Path.Combine(currentDir, loadInput);
                                }

                                if (!loadPath.EndsWith(".txt"))
                                    loadPath += ".txt";

                                if (File.Exists(loadPath))
                                {
                                    string[] lines =
                                        File.ReadAllLines(loadPath);

                                    SaveState();

                                    currentFile = loadPath;

                                    for (int y = 0;
                                        y < height &&
                                        y < lines.Length;
                                        y++)
                                    {
                                        for (int x = 0;
                                            x < width &&
                                            x < lines[y].Length;
                                            x++)
                                        {
                                            canvas[y, x] =
                                                lines[y][x];
                                        }
                                    }
                                }
                            }

                            Console.CursorVisible = false;

                            Console.Clear();
                        }
                        catch
                        {
                            Console.CursorVisible = false;
                            Console.Clear();
                        }

                        break;

                    // exit
                    case ConsoleKey.Escape:
                    case ConsoleKey.Q:

                        running = false;

                        break;
                }
            }

            Console.CursorVisible = true;

            Console.Clear();

            Console.ForegroundColor = ConsoleColor.Cyan;
            Console.WriteLine("exited fisdraw");
        }
    }
}
