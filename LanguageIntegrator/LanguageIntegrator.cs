// Copyright (c) Ivan Bondarev, Stanislav Mikhalkovich (for details please see \doc\copyright.txt)
// This code is distributed under the GNU LGPL (for details please see \doc\license.txt)

using System;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text;
using Languages.Facade;
using PascalABCCompiler;

namespace Languages.Integration
{
    /// <summary>
    /// Модуль с функциями загрузки поддерживаемых языков программирования
    /// </summary>
    public static class LanguageIntegrator
    {

        private static readonly LanguageProvider LanguageProvider = LanguageProvider.Instance;

        /// <summary>
        /// Событие, информирующее об успешной загрузке языкового пакета
        /// </summary>
        public static event Action<ILanguage> LanguageLoaded;

        /// <summary>
        /// Событие, информирующее об ошибке загрузки языкового пакета
        /// </summary>
        public static event Action<string> LanguageLoadErrorOccured;

        // Release builds must preserve the exception, not only the DLL name.
        public static event Action<string, Exception> LanguageLoadFailed;

        /// <summary>
        /// Загружает все языки платформы из папки bin
        /// </summary>
        public static void LoadAllLanguages()
        {
#if PABCNET_MODERN
            Encoding.RegisterProvider(CodePagesEncodingProvider.Instance);
#endif
            string directoryName = Path.GetDirectoryName(Assembly.GetExecutingAssembly().ManifestModule.FullyQualifiedName);

            DirectoryInfo directory = new DirectoryInfo(directoryName);

            FileInfo[] dllFiles = directory.GetFiles("*LanguageInfo.dll");

            var mainLanguageDll = dllFiles.FirstOrDefault(fileInfo =>
                string.Equals(fileInfo.Name, StringConstants.pascalLanguageDllName, StringComparison.OrdinalIgnoreCase));
            if (mainLanguageDll == null)
                throw new FileNotFoundException("Не найден основной языковой модуль PascalABC.NET. Распакуйте сборку целиком.",
                    Path.Combine(directoryName, StringConstants.pascalLanguageDllName));

            // первым добавляем PascalABC.NET
            var mainLanguageError = IntegrateLanguageFromAssembly(mainLanguageDll);
            if (mainLanguageError != null)
                throw mainLanguageError;

            foreach (FileInfo languageFile in dllFiles.Where(dll => dll != mainLanguageDll))
            {
                IntegrateLanguageFromAssembly(languageFile);
            }
        }

        /// <summary>
        /// Загружает плагин языка из dll, содержащей класс, представляющий язык 
        /// (класс должен реализовывать интерфейс ILanguage и его имя должно заканчиваться на Language)
        /// </summary>
        /// <param name="languageFile">dll, содержащая язык</param>
        private static Exception IntegrateLanguageFromAssembly(FileInfo languageFile)
        {
            ILanguage languageFound;
            try
            {
                languageFound = InstantiateLanguageAndAddToLanguagesList(languageFile);

                if (languageFound == null)
                {
                    throw new Exception("Language class wasn't found in language assembly. (To be found it should contain 'Language' in it's name)");
                }
            }
            catch (Exception e)
            {
                var message = "Не удалось загрузить языковой модуль " + languageFile.Name + ".";
                if (IsRemoteSourceBlocked(e))
                    message += " Windows ограничила загрузку сборки (0x80131515). Возможная причина — метка скачивания из интернета. " +
                        "Если вы доверяете источнику ZIP, откройте свойства исходного архива, нажмите «Разблокировать» и распакуйте его заново в новую папку.";
                var failure = new InvalidOperationException(message, e);
                // Diagnostics subscribers cannot replace the original load error.
                if (LanguageLoadFailed != null)
                    foreach (Action<string, Exception> observer in LanguageLoadFailed.GetInvocationList())
                        try { observer(languageFile.FullName, failure); } catch { }
                LanguageLoadErrorOccured?.Invoke(languageFile.Name);
                return failure;
            }
            LanguageLoaded?.Invoke(languageFound);
            return null;
        }

        private static bool IsRemoteSourceBlocked(Exception error)
        {
            if (error == null) return false;
            if (error is FileLoadException && error.HResult == unchecked((int)0x80131515)) return true;
            var typeLoad = error as ReflectionTypeLoadException;
            if (typeLoad?.LoaderExceptions != null && typeLoad.LoaderExceptions.Any(IsRemoteSourceBlocked)) return true;
            return IsRemoteSourceBlocked(error.InnerException);
        }

        /// <summary>
        /// Конструирует объект языка и добавляет его в список языков в LanguageProvider
        /// </summary>
        /// <param name="languageFile"></param>
        /// <returns></returns>
        private static ILanguage InstantiateLanguageAndAddToLanguagesList(FileInfo languageFile)
        {
            // Assembly assembly = Assembly.LoadFile(languageFile.FullName);
            // SSM 02.01.2026 Поменял для net 9.0 иначе падение - не загружался PascalABCParser
            Assembly assembly = Assembly.LoadFrom(languageFile.FullName);

            Type[] types = assembly.GetTypes();

            ILanguage languageFound = null;
            foreach (Type type in types)
            {
                if (type.Name.EndsWith("Language", StringComparison.Ordinal) && type.IsClass &&
                    !type.IsAbstract && typeof(ILanguage).IsAssignableFrom(type))
                {
                    object obj = Activator.CreateInstance(type);
                    if (obj is ILanguage language)
                    {
                        languageFound = language;
                        LanguageProvider.Languages.Add(language);
                    }
                    break;
                }
            }

            return languageFound;
        }
    }
}
