package require Tk 8.6

namespace eval app {
    variable filename ""
    variable dirty 0
    variable channel ""
    variable snapshot ""
    variable input ""
    variable output ""
    variable highlightJob ""
    variable stopped 0
    variable example {begin
  var n: integer;
  Write('Введите число: ');
  Readln(n);
  Writeln('Результат: ', n * 2);
end.
}
}

proc app::appendOutput {text} {
    variable output
    append output $text
    .panes.console.text configure -state normal
    .panes.console.text insert end $text
    .panes.console.text see end
    .panes.console.text configure -state disabled
}

proc app::setDocument {text} {
    variable dirty
    .panes.source.text delete 1.0 end
    .panes.source.text insert 1.0 $text
    .panes.source.text edit reset
    .panes.source.text edit modified 0
    set dirty 0
    highlight
}

proc app::save {} {
    variable filename
    variable dirty
    set target $filename
    if {$target eq ""} { set target [tk_getSaveFile -defaultextension .pas -filetypes {{{Pascal} {.pas}}}] }
    if {$target eq ""} { return 0 }
    set temporary ""
    if {[catch {
        set fd [file tempfile temporary [file join [file dirname $target] .abc-save-]]
        try {
            fconfigure $fd -encoding utf-8 -translation lf
            puts -nonewline $fd [.panes.source.text get 1.0 end-1c]
        } finally { close $fd }
        file rename -force $temporary $target
        set temporary ""
    } message]} {
        if {$temporary ne ""} { catch {file delete -force $temporary} }
        tk_messageBox -icon error -title "Сохранение" -message $message
        return 0
    }
    set filename $target
    set dirty 0
    .panes.source.text edit modified 0
    return 1
}

proc app::confirmSave {} {
    variable dirty
    if {!$dirty} { return 1 }
    set answer [tk_messageBox -type yesnocancel -default yes -icon question -title "Изменения" -message "Сохранить изменения программы?"]
    if {$answer eq "cancel"} { return 0 }
    if {$answer eq "yes"} { return [save] }
    return 1
}

proc app::new {} {
    variable filename
    variable example
    if {![confirmSave]} { return }
    set filename ""
    setDocument $example
}

proc app::openFile {} {
    variable filename
    if {![confirmSave]} { return }
    set target [tk_getOpenFile -filetypes {{{Pascal} {.pas}}}]
    if {$target eq ""} { return }
    if {[catch {
        if {[file size $target] > 1048576} { error "Файл превышает 1 МиБ" }
        set fd [open $target r]
        try {
            fconfigure $fd -encoding utf-8 -translation auto
            set text [read $fd]
        } finally { close $fd }
    } message]} { tk_messageBox -icon error -message $message; return }
    set filename $target
    setDocument $text
}

proc app::running {value} {
    foreach name {check run} { .toolbar.$name configure -state [expr {$value ? "disabled" : "normal"}] }
    .toolbar.stop configure -state [expr {$value ? "normal" : "disabled"}]
    .input configure -state [expr {$value ? "normal" : "disabled"}]
    .status configure -text [expr {$value ? "Выполняется…" : "Готово"}]
}

proc app::start {command} {
    variable channel
    variable snapshot
    variable filename
    variable output
    variable stopped
    if {$channel ne ""} { return }
    set stopped 0
    set output ""
    .panes.console.text configure -state normal
    .panes.console.text delete 1.0 end
    .panes.console.text configure -state disabled
    if {[catch {
        set fd [file tempfile snapshot]
        try {
            fconfigure $fd -encoding utf-8 -translation lf
            puts -nonewline $fd [.panes.source.text get 1.0 end-1c]
        } finally { close $fd }
        set previous [pwd]
        try {
            if {$filename ne ""} { cd [file dirname $filename] }
            set pipeline [list | $::abc_runner $command $snapshot 2>@1]
            set channel [open $pipeline r+]
        } finally { cd $previous }
        fconfigure $channel -blocking 0 -buffering none -encoding utf-8 -translation lf
        fileevent $channel readable app::receive
        running 1
        if {$::abc_smoke} { puts $channel 21; flush $channel }
    } message]} {
        appendOutput "Не удалось запустить ядро: $message\n"
        if {$channel ne ""} { catch {close $channel}; set channel "" }
        if {$snapshot ne ""} { file delete -force $snapshot; set snapshot "" }
        running 0
        if {$::abc_smoke} { exit 1 }
    }
}

proc app::receive {} {
    variable channel
    variable snapshot
    variable output
    if {$channel eq ""} { return }
    if {[catch {read $channel 8192} chunk]} { set chunk "" }
    if {$chunk ne ""} { appendOutput $chunk }
    if {![eof $channel]} { return }
    fileevent $channel readable {}
    # At EOF our runner has completed. Blocking close collects its exit status.
    fconfigure $channel -blocking 1
    set code [catch {close $channel} message options]
    set channel ""
    if {$snapshot ne ""} { file delete -force $snapshot; set snapshot "" }
    set exitCode 0
    if {$code} {
        set exitCode 1
        if {[dict exists $options -errorcode] && [lindex [dict get $options -errorcode] 0] eq "CHILDSTATUS"} {
            set exitCode [lindex [dict get $options -errorcode] 2]
        }
    }
    appendOutput "\n\[Завершено, код $exitCode\]\n"
    running 0
    if {$::abc_smoke} {
        set success [expr {$code == 0 && [string first "Результат: 42" $output] >= 0}]
        after 100 [list exit [expr {$success ? 0 : 1}]]
    }
}

proc app::sendInput {} {
    variable channel
    variable input
    if {$channel eq ""} { return }
    if {[catch {puts $channel $input; flush $channel} message]} { appendOutput "$message\n" }
    set input ""
}

proc app::stop {} {
    variable channel
    variable stopped
    if {$channel eq ""} { return }
    set stopped 1
    foreach id [pid $channel] {
        if {$::tcl_platform(platform) eq "windows"} { catch {exec taskkill /PID $id /T /F} }
        else { catch {exec kill -TERM $id} }
    }
}

proc app::closeWindow {} {
    variable channel
    variable snapshot
    if {![confirmSave]} { return }
    stop
    if {$channel ne ""} { catch {close $channel} }
    if {$snapshot ne ""} { catch {file delete -force $snapshot} }
    destroy .
}

proc app::changed {} {
    variable dirty
    variable highlightJob
    if {![.panes.source.text edit modified]} { return }
    set dirty 1
    .panes.source.text edit modified 0
    if {$highlightJob ne ""} { after cancel $highlightJob }
    set highlightJob [after 150 app::highlight]
}

proc app::highlight {} {
    variable highlightJob
    set highlightJob ""
    set widget .panes.source.text
    foreach tag {keyword literal comment} { $widget tag remove $tag 1.0 end }
    set text [$widget get 1.0 end-1c]
    set pattern {(?i)(?:'(?:''|[^'\r\n])*'|//[^\n]*|\{[^\}]*\}|\(\*(?:[^*]|\*+[^*)])*\*+\)|\m(?:begin|end|program|var|integer|int64|real|boolean|string|if|then|else|while|do|and|or|not|div|mod|true|false)\M)}
    foreach range [regexp -all -inline -indices $pattern $text] {
        lassign $range first last
        set token [string range $text $first $last]
        set tag keyword
        if {[string index $token 0] eq "'"} { set tag literal }
        if {[string match {//*} $token] || [string index $token 0] eq "\{" || [string range $token 0 1] eq "(*"} { set tag comment }
        $widget tag add $tag "1.0 + $first chars" "1.0 + [expr {$last + 1}] chars"
    }
}

wm title . "ABC Native — Tcl/Tk — прототип"
wm geometry . 1100x760
wm minsize . 650 420
wm protocol . WM_DELETE_WINDOW app::closeWindow
ttk::frame .toolbar -padding 6
foreach {name text command} {
    new Новый app::new open Открыть app::openFile save Сохранить app::save
    check Проверить {app::start check} run Запустить {app::start run} stop Остановить app::stop
    about {О проекте} {tk_messageBox -title "ABC Native" -message "Экспериментальный редактор Pascal на Tcl/Tk.\nЯдро: C++20.\n\nАвторы PascalABC.NET:\nIvan Bondarev, Stanislav Mikhalkovich.\nЛицензии находятся в каталоге licenses поставки."}
} {
    ttk::button .toolbar.$name -text $text -command $command
    pack .toolbar.$name -side left -padx 3
}
pack .toolbar -side top -fill x
ttk::label .status -padding 6 -text "Pascal • C++ ядро • F8 проверка • F9 запуск"
pack .status -side bottom -fill x
ttk::entry .input -textvariable app::input
pack .input -side bottom -fill x -padx 8 -pady 6
ttk::panedwindow .panes -orient vertical
foreach {name readonly} {source 0 console 1} {
    ttk::frame .panes.$name
    text .panes.$name.text -wrap none -undo [expr {!$readonly}] -font TkFixedFont -padx 10 -pady 8 -borderwidth 1
    ttk::scrollbar .panes.$name.scroll -orient vertical -command [list .panes.$name.text yview]
    .panes.$name.text configure -yscrollcommand [list .panes.$name.scroll set]
    ttk::scrollbar .panes.$name.horizontal -orient horizontal -command [list .panes.$name.text xview]
    .panes.$name.text configure -xscrollcommand [list .panes.$name.horizontal set]
    grid .panes.$name.text -row 0 -column 0 -sticky nsew
    grid .panes.$name.scroll -row 0 -column 1 -sticky ns
    grid .panes.$name.horizontal -row 1 -column 0 -sticky ew
    grid rowconfigure .panes.$name 0 -weight 1
    grid columnconfigure .panes.$name 0 -weight 1
    .panes add .panes.$name -weight [expr {$readonly ? 1 : 3}]
}
.panes.source.text tag configure keyword -foreground #2256a5
.panes.source.text tag configure literal -foreground #92522c
.panes.source.text tag configure comment -foreground #64766b
.panes.console.text configure -state disabled
pack .panes -fill both -expand 1 -padx 8
bind .panes.source.text <<Modified>> app::changed
bind .input <Return> app::sendInput
bind . <F8> {app::start check}
bind . <F9> {app::start run}
bind . <Control-s> app::save
app::setDocument $app::example
app::running 0
focus .panes.source.text
if {$::abc_smoke} {
    after 0 {app::start run}
    after 15000 {exit 1}
}
