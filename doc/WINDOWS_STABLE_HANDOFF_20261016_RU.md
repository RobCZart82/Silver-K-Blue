# Windows: стабильная основа Silver-K и сравнение с OsTIrus

Дата: 16 октября 2026. Ветка доставки: `codex/stable-windows-optimization`.

## Решение пользователя и цель Windows-продолжения

Пользователь временно заморозил полный перенос контроллера KS. Нужен рабочий
инструмент на последней основе с продуктовыми проверками и продолжение
оптимизации именно под Windows, включая новое сравнение с OsTIrus на Windows.
Старый goal «доделать полный перенос, затем CPU и push» не возобновляется.

Начать с этой ветки, собрать неизменённую основу, проверить звук/состояние,
получить Windows baseline и сравнение с OsTIrus. После этого делать небольшие
Windows-оптимизации с отдельными before/after-проверками и записью в журнал.
Нельзя считать Mac-сборку, аудиопроверки или CPU-цифры Windows-доказательством.

## Что передаётся

- Основа `1724c1b` — последний сохранённый этап с полным Mac продуктовым
  циклом до последующей семантической миграции контроллера.
- Единственное новое production-изменение доставки: при отсутствии переменных
  окружения включены уже проверенные event DSP transport и control jobs.
  Алгоритмы этих путей не переписывались. Явные переопределения сохранены:
  `SILVER_KS_DSP_TRANSPORT=legacy`, `SILVER_KS_CONTROL_JOBS=0` отключают их.
- Production KS с reset работает через C++ compatibility controller.
  Оригинальный H8 входит только в сравнительные тесты. Регистры/CCR/EXR/
  SP/стек/PC и совместимая диспетчеризация остаются. Это рабочий промежуточный
  контроллер, а не законченный перенос без модели CPU.
- Поздний checkpoint `909aa2bff4cb4e42872bf01baf8ae5e1e81af8f5` сохранён
  отдельно на Mac. Его обработчики и CPU-free графы сюда не переносить.
- DSP firmware/JIT, гостевые clocks/slices/IRQ/serial B→A, resampling, размеры
  буферов и приоритеты потоков в доставке не менялись. По умолчанию DSP
  исполняются последовательно. K-Station остаётся отдельным движком.

Сначала прочитать `README.md`, эту заметку, последние датированные записи
`doc/OPTIMIZATION_LOG.md` и `doc/PERFORMANCE_DIAGNOSTICS.md`. Исторические
инструкции о H8 default/selector и старые TODO не отменяют остановку переноса.

## Что реально проверено на Mac

macOS 27.2 / Apple M1 Pro / arm64 Release / AppleClang 21 / C++17,
profiling OFF, фиксированные зависимости из `cmake/Dependencies.cmake`.

- Заново собраны AU, VST3, Standalone; подписи и arm64 проверены.
- CTest: 24 PASS, семь skips без приватных входов, ноль незакрытых ошибок.
  Пять исключённых тестовых целей зависимостей сначала отсутствовали;
  после их сборки все пять успешно выполнены.
- Приватные Single/Multi cold-start, runtime и controller/state проверки PASS.
- AU Single/Multi+FX: новая версия без env-флагов совпала с чистой основой,
  где оптимизации явно включены, по звуку, хвостам и сохранённым патчам.
  Все части звучали; пропусков кадров и deadline failures нет.
- Реальные VST3 realtime/offline note/editor проверки PASS.
- Standalone открыл окно. Восстановление старого состояния JUCE-хоста показало
  устаревшие пути OS/Wave; его физическое аудиоустройство отдельно не проверено.
- AU установлен в пользовательскую папку на Mac; прежний AU сохранён локально.
  Это не установка и не проверка чего-либо на Windows.

Точный AU SHA-256: `5a85eae3124a55b9e5f2a1082ff1a80ebdb178d9477b1003a7f794b761d72ca8`.
Бинарник собран до delivery-коммита: embedded identity `1724c1b` + dirty.
После сборки менялись только документы/continuity instructions; звуковой код
доставки соответствует двум default-selector изменениям в этой ветке.

Повторный Mac AU тест: 48 kHz/256, 16 нот всего, базовые saw-патчи,
Single либо четыре части по четыре ноты, dry либо delay/reverb/chorus,
три чередующиеся пары по восемь секунд; 24 окна /192 measured seconds.
CPU = user+system всего процесса, 100% = одно ядро.

| Сценарий | Silver-K median | OsTIrus 2.2.16/TI2 median |
| --- | ---: | ---: |
| Single dry | 46.703% | 73.485% |
| Single FX | 45.271% | 72.702% |
| Multi dry | 45.469% | 69.141% |
| Multi FX | 44.760% | 84.010% |

Все 24 окна конечные/слышимые, с проверенными параметрами и каналами,
без over-budget callbacks. У Silver-K во всех 12 окнах missing_frames=0,
аудио и хвосты каждого сценария одинаковы между повторами. Внутренний
lost-frame counter OsTIrus недоступен, это не измеренный ноль.
Диапазоны и ограничения: `doc/STABLE_PERFORMANCE_20261016_RU.md`.

Была фоновая активность mds/RustDesk, частоты/температуры/core placement не
измерялись. Синтез, родные частоты и эффекты у инструментов различаются.
Это не одинаковая DSP-работа, не универсальный рейтинг и не результат для
Windows. Более ранние абсолютные серии с этими цифрами не объединять.

## Получение и сборка Windows x64

Не перезаписывать текущую работу Windows-чата. Получить remote branch,
использовать отдельный worktree/checkout при наличии локальных изменений.
Никаких reset/clean существующего рабочего дерева.

```powershell
git fetch origin codex/stable-windows-optimization
# В чистом подходящем checkout:
git switch --track origin/codex/stable-windows-optimization
cmake -S source/novation/silverK/buildStandalone -B build/windows-stable -A x64
cmake --build build/windows-stable --config Release --parallel 4
```

Нужны Visual Studio с Desktop development with C++, Windows SDK,
CMake >=3.22 и Python 3. Для offline-сборки передать `JUCE_SOURCE`,
`DSP_SOURCE`, `ASMJIT_SOURCE` с исходниками ровно закреплённых revisions.
Не подменять JIT или compiler optimization policy для получения преимущества.

Собрать также EXCLUDE_FROM_ALL fixtures и пять dependency tests, иначе полный
CTest может получить Not Run вместо действительного результата:

```powershell
cmake --build build/windows-stable --config Release --parallel 4 --target ksRuntimeController_test ksLiveMidiController_test ksControllerBoot_test ksControllerUart_test ksControllerTiming_test ksDspEquivalence_test silverRealtime_test silverController_test silverCpuCost_test ksCpuCost_test sharedAudioBufferTest sharedAudioReducerTest ringBufferTest semaphoreTest profilerTest
$env:SILVER_K_DATA = Join-Path $env:TEMP 'silver-k-windows-stable-tests'
ctest --test-dir build/windows-stable -C Release --output-on-failure
python tools/check_repository.py
```

Использовать новые отдельные каталоги для разных тестов/повторов. Skip77
означает отсутствие приватных входов, а не успешную аудиопроверку.
С локальными KS OS2.1.02, Flash и Wave отдельно выполнить холодный запуск
Single/Multi, runtime/controller, MIDI/pedal/steal/FX/state/SysEx/Flash проверки
и тесты настоящих Windows VST3/Standalone. Пути executable в multi-config
обычно `build/windows-stable/silverPlugin/Release/`; подтвердить фактические.
Для K-Station требуется отдельная K OS2.1.01, не выдавать KS-проверку за K.

## Windows baseline и сравнение с OsTIrus

1. Сначала зафиксировать исходный commit, dirty state, SHA-256 реальных
   VST3/Standalone и измерительного хоста, зависимости/compiler/config,
   Windows build, CPU/P/E/SMT, host/driver/device, actual/prepared buffer,
   sample rate/latency/reserve, число частей/нот/инстансов и UI state.
2. Для Silver-K before/after уже есть Windows CPU accounting в
   `silverCpuCost_test.cpp`: GetProcessTimes считает user+kernel всех потоков,
   GetThreadTimes — caller. `tools/run_cpu_cost_comparison.py` обслуживает
   сравнение собственных режимов KS; это НЕ готовый общий OsTIrus host.
3. Mac `auCpuBenchmark.cpp` использует AudioToolbox и на Windows не запускается.
   Для прямого Silver-K/OsTIrus сравнения использовать один Windows VST3 host
   с одинаковым pacing и GetProcessTimes вокруг измеряемого окна. При
   необходимости сделать узкую Windows-версию измерительного хоста на основе
   `vst3_check.cpp`, сохраняя измерительный код вне production audio callback.
   Нельзя сравнивать worker wall ratio одного продукта с process CPU другого.
4. Повторить четыре сценария выше: basic saw, без unison/arp, Single16 и
   Multi4×4, dry и delay/reverb/chorus. Проверить фактически применённые
   параметры, каналы, число частей, finite/non-silent звук и FX tails.
   Записать настоящую Windows-версию OsTIrus и ROM/model TI/TI2/Snow.
   Это разные модели нагрузки; Snow не является dual-DSP TI2 контролем.
5. Не менее трёх чередующихся пар по сценарию; прогрев/подготовка/снимки state
   вне окна. Начать с 48 kHz/256, 16 нот суммарно, profiling/diagnostics OFF.
   CPU% = 100 × delta process CPU seconds / delivered audio seconds,
   100%=одно ядро. Не делить ещё раз на количество логических CPU.
6. Сначала correctness, затем CPU. Кандидаты Silver-K должны совпасть с
   собственной baseline по полным commands/audio/state в принятых сценариях;
   нельзя требовать одинакового PCM от Silver-K и Virus. Отбраковывать
   потери кадров, over-budget callbacks, тишину, неправильные parts/FX.
   Недоступный счётчик OsTIrus помечать unavailable, а не нулём.
7. Без параллельных сборок/stack sampling/профилирования. Указать реальный
   фон и наблюдаемые ограничения. Не менять Windows power policy, process-wide
   affinity, thread priorities, JIT, buffer sizes или DSP clocks ради сравнения.
   Профиль/ETW — отдельный диагностический запуск, не CPU evidence.

## Разрешённое продолжение оптимизации

После проверенной Windows baseline — небольшие Windows worker wake-up/idle
изменения в прежней области разрешения AGENTS. Сохранять default sequential
DSP, guest instruction/IRQ/serial ordering, resampling и буферы. Опциональные
KS worker CPU Sets/dual-DSP эксперименты остаются opt-in, с исходным контролем,
реальным ETW placement и звуковым доказательством до оценки CPU. Их прежнее
разрешение не даёт права изменить OS/process-wide policy или продолжить
замороженный полный перенос контроллера. Mac wait behavior не менять.

Каждый принятый этап: новый dated Windows entry в OPTIMIZATION_LOG с настоящими
build/test IDs, baseline/candidate, медианами/диапазонами и ограничениями.
Отклонённые гипотезы не выдавать за улучшения. Не переносить Mac CPU claims.

## Приватность, очистка, состояние сохранения

Пользователь запросил удалить ненужные WAV/raw traces, логи, тестовые продукты
и временные build caches после остановки. На Mac сохраняются готовый пакет,
публичный краткий отчёт, backup установленного AU и source-only stop archive
для замороженной миграции. Старые абсолютные private report/scratch paths из
журнала могут больше не существовать. Это не поставка измерительных бинарников
или сырых доказательств на Windows; тесты нужно реально повторить.

Firmware, банки, PCM, полные traces, бинарники и персональные отчёты не
коммитить/не push. Собственные ROM/банки Windows хранить вне Git; публиковать
санитизированные выводы и тестовые идентификаторы. Соседний preset-manager
checkout не трогать и автоматически не merge. Сначала получить рабочую
Windows-сборку и измерения этой ветки, затем решать о следующем изменении.

Короткий стартовый запрос следующему Windows-чату:

> Продолжи Windows performance work на codex/stable-windows-optimization.
> Прочитай этот handoff и AGENTS. Полный KS controller port заморожен.
> Собери стабильную основу, реально проверь Windows звук/state, получи
> matched process-CPU comparison с установленным OsTIrus на Windows.
> Затем оптимизируй только в разрешённой области с before/after-проверками
> и дневником. Не меняй DSP/JIT/guest timing/Windows policy и не объявляй
> Mac-результаты Windows-доказательством. Приватные материалы не публикуй.
