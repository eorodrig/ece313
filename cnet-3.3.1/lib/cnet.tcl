#
# Tcl/Tk windowing routines for the cnet network simulator (v3.3.1)
# Copyright (C) 1992-onwards, Chris.McDonald@uwa.edu.au
# with thanks to Michael J. Robins.
# Released under the GNU General Public License (GPL) version 2.

if {$tk_version < 8.4} \
    then {puts "sorry, Tcl/Tk v8.4 or greater required" ; exit}
#
# All of the following constants may be safely modified:
#
if { $tcl_platform(os) == "Darwin" } {		# Mac-OSX
    font create myfont -family "Helvetica" -size 10 -weight normal
    font create ttfont -family "Monaco" -size 11 -weight normal
    set modifier	Command
} else {					# all Linux/Unix
    font create myfont -family "Helvetica" -size 8 -weight normal
    font create ttfont -family "Courier" -size 8 -weight normal
    set modifier	Control
}
#
set framebg	 	"#cccccc"
set canvasbg 		"#e6ffe6"
set canvasfg	 	black
set stdiocols	 	80
set stdiorows	 	24
set stdiohistory 	64
set tracerows	 	32
set tracecols	 	90
set tracehistory 	100
#
# ----------------------------------------------------------------
# No simple modifications below here
#
set fw		[font measure ttfont m]
set fh		[expr [font metrics ttfont -linespace] + 2]
set map		""

proc newToplevel {w str deleteproc} {
    toplevel $w
    wm withdraw $w
    wm title $w "$str"
    wm protocol $w WM_DELETE_WINDOW "$deleteproc"
    update idletasks
}

proc newCanvas {w1 w2 width height} {
    global	framebg canvasbg

    set cf [frame $w1.cf -background $framebg]
    canvas $cf.$w2 -width $width -height $height  \
	-background $canvasbg -relief sunken -bd 2
    pack $cf.$w2 -fill both -expand yes
    pack $cf -fill both -expand yes
}

proc newText {w rows cols history} {
    global	canvasbg canvasfg

    set t $w.stdio
    text $t \
	-font		ttfont \
	-width		$cols \
	-height		$rows \
	-background	$canvasbg \
	-foreground	$canvasfg \
	-relief		sunken \
	-bd		2 \
	-undo		false \
	-wrap		none \
	-yscrollcommand "$w.vscroll set"
    $t tag configure bg0 -background $canvasbg -foreground $canvasfg
    $t tag configure bg1 -background white
    $t tag configure bg2 -background red -foreground white

    scrollbar $w.vscroll -orient vertical -command "$t yview"
    pack $w.vscroll -side right -fill both -expand no
    pack $t -side left -fill both -expand yes
    for {set l1 1} {$l1 <= $history} {incr l1} {
	$t insert $l1.0 "\n"
    }
    $t see $history.0
    return $t
}

# ----------------------------------------------------------------

proc initworld {topology bg mapx mapy gflag maxspeed procs
		nictypes nicdescs nevents nstats} {
    global	framebg canvasbg
    global	modifier
    global	CN_POSITION CN_TIMENOW CN_DELIVERY
    global	map

    wm withdraw .
    wm protocol . WM_DELETE_WINDOW [lindex $procs 0]
    wm title . "cnet: $topology"

    if {$bg != ""} {
	set canvasbg	"$bg"
    }
    set CN_POSITION	"0,0"
    set CN_TIMENOW	"0usec"
    set CN_DELIVERY	"0 (100.0%)"

    frame .cnet -background white

# create the menubar
    menu .cnet.mb -type menubar
    set m [menu .cnet.mb.cnet  -tearoff 0]
    $m add command -label "$gflag" \
	    -command "[lindex $procs 1] run" \
	    -accelerator "space"
    $m add command -label "single step" \
	    -command "[lindex $procs 1] step" \
	    -accelerator "Return"
    $m add command -label "save topology" \
	    -command "toggleSave [lindex $procs 2]" \
	    -accelerator "$modifier-s"
    $m add separator
    $m add command -label "Quit cnet" \
	    -command "[lindex $procs 0]" \
	    -accelerator "$modifier-q"
    .cnet.mb add cascade -menu $m -label "Run"

    set m [menu .cnet.mb.setspeed  -tearoff 0]
    $m add radio -label "tortoise"	-command "scheduler_speed 1"
    $m add radio -label "1/8"		-command "scheduler_speed 2"
    $m add radio -label "1/4"		-command "scheduler_speed 3"
    $m add radio -label "1/2"		-command "scheduler_speed 4"
    $m add radio -label "normal"	-command "scheduler_speed 5"
    $m add radio -label "2x"		-command "scheduler_speed 6"
    $m add radio -label "4x"		-command "scheduler_speed 7"
    $m add radio -label "8x"		-command "scheduler_speed 8"
    $m add radio -label "hare"		-command "scheduler_speed 9"
    if {$maxspeed} then {
	$m invoke 8
    } else {
	$m invoke 4
    }
    .cnet.mb add cascade -menu .cnet.mb.setspeed -label "Run-speed"

    set m [menu .cnet.mb.setupdate  -tearoff 0]
    $m add radio -label "100usec" 	-command "update_speed 100u"
    $m add radio -label "1msec" 	-command "update_speed 1ms"
    $m add radio -label "100msec" 	-command "update_speed 100ms"
    $m add radio -label "1sec" 		-command "update_speed 1s"
    $m add radio -label "10sec" 	-command "update_speed 10s"
    $m add radio -label "60sec" 	-command "update_speed 60s"
    $m invoke 3

    .cnet.mb add cascade -menu .cnet.mb.setupdate -label "Update-freq"

    set m [menu .cnet.mb.subwindows  -tearoff 0]
    $m add command -label "events" \
		-command "toggleEvents $topology $nevents" \
		-accelerator "e"
    $m add command -label "statistics" \
		-command "toggleStats $topology $nstats" \
		-accelerator "s"
    $m add command -label "trace" \
		-command "toggleTrace $topology" \
		-accelerator "t"
    $m add separator
    $m add command -label "default node attributes" \
	    	-command "toggleNode default 0 0"
    if {[lindex $nictypes 0] > 0} then {
	set desc [lindex $nicdescs 0]
	set nicname "default WAN attributes"
	$m add command -label $nicname \
	    -command "toggleNIC WAN -1 -1 \"$nicname\" 0 0 \"$desc\""
    }
    if {[lindex $nictypes 1] > 0} then {
	set desc [lindex $nicdescs 1]
	set nicname "default LAN attributes"
	$m add command -label $nicname \
	    -command "toggleNIC LAN -1 -1 \"$nicname\" 0 0 \"$desc\""
    }
    if {[lindex $nictypes 2] > 0} then {
	set desc [lindex $nicdescs 2]
	set nicname "default WLAN attributes"
	$m add command -label $nicname \
	    -command "toggleNIC WLAN -1 -1 \"$nicname\" 0 0 \"$desc\""
    }
    .cnet.mb add cascade -menu $m -label "Subwindows"
    . configure -menu .cnet.mb

# create the canvas for the map
    set sw  [expr [winfo screenwidth .]  -  40]
    set sh  [expr [winfo screenheight .] - 160]
    set mf  [frame .cnet.mapframe]
#
    set map [canvas $mf.map \
	    -width  [expr ($mapx > $sw) ? $sw : $mapx] \
	    -height [expr ($mapy > $sh) ? $sh : $mapy] \
	    -background $canvasbg  \
	    -scrollregion "0 0 $mapx $mapy"  \
	    -yscrollcommand "$mf.ysbar set"  \
	    -xscrollcommand "$mf.xsbar set" ]
    scrollbar $mf.ysbar -orient vertical   -command {$map yview}
    scrollbar $mf.xsbar -orient horizontal -command {$map xview}

# create the statusbar frame
    set f [frame .cnet.status -background $framebg]
    label $f.position \
	-background $framebg \
	-width 10 \
	-anchor w \
	-textvariable CN_POSITION
    label $f.time -text "Simulation time:" -background $framebg
    label $f.timenow \
	-background $framebg \
	-width 18 \
	-anchor w \
	-textvariable CN_TIMENOW
    label $f.msgs -text "Messages delivered:" -background $framebg
    label $f.delok \
	-background $framebg \
	-width 16 \
	-anchor c \
	-textvariable CN_DELIVERY

# pack everything
    pack $f.position $f.time $f.timenow $f.msgs $f.delok -side left
    pack $mf.ysbar -side right -fill y
    pack $mf.xsbar -side bottom -fill x
    pack $mf.map -fill both -expand yes

    pack .cnet.mapframe -fill both -expand yes
    pack .cnet.status -side bottom -fill both
    pack .cnet  -fill both -expand yes

# define all keyboard shortcuts
    bind . "<Key-e>"		"toggleEvents $topology $nevents"
    bind . "<Key-s>"		"toggleStats $topology $nstats"
    bind . "<Key-t>"		"toggleTrace $topology"

    bind . "<Escape>"		"[lindex $procs 0]"
    bind . "<Key-q>"		"[lindex $procs 0]"
    bind . "<$modifier-q>"	"[lindex $procs 0]"
    bind . "<space>"		"[lindex $procs 1] run"
    bind . "<Return>"		"[lindex $procs 1] step"
    bind . "<$modifier-s>"	"toggleSave [lindex $procs 2]"
    bind . "<Key-w>"		"[lindex $procs 3]"

    bind . "<MouseWheel>" {
	if {%D > 0} {set u -1} else {set u 1}
	$map yview scroll $u units
    }
    bind $map "<Motion>" {
	mouse_position [%W canvasx %x] [%W canvasy %y]
    }

    update idletasks
    wm deiconify .
}

proc initPopup {w title deleteproc width height} {
    global	framebg

    newToplevel $w "$title" "$deleteproc"

    set f [frame $w.top]
    label $f.update -background $framebg \
		-textvariable "UPDATE_TITLE"
    pack $f.update -side left -fill both -expand yes
    pack $f -side top -fill both -expand yes

    newCanvas $w can $width $height
    wm resizable $w 0 0
}

# ----------------------------------------------------------------

proc toggleTrace {topology} {
    global	tracerows tracecols tracehistory
    global	trace_displayed

    set w .trace
    if {[info command $w] == ""} {
	newToplevel $w "$topology trace" "toggleTrace $topology"
	newText $w $tracerows $tracecols $tracehistory
    }
    if {[wm state $w] == "withdrawn"} {
	update idletasks
	wm deiconify $w
	raise $w
	set trace_displayed true
    } else {
	wm withdraw $w
	set trace_displayed false
    }
}

proc traceoutput {str bg nl} {
    global	tracehistory

    set t .trace.stdio
    $t insert $tracehistory.end "$str" "$bg"
    if {$nl} then {
	$t insert $tracehistory.end \
		    "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t" "$bg"
	for {set l1 1; set l2 2} {$l1 <= $tracehistory} {incr l1; incr l2} {
	    $t delete $l1.0 $l1.end
	    set ts [$t tag names $l2.0]
	    $t insert $l1.0 [$t get $l2.0 $l2.end] $ts
	}
    }
}

# ----------------------------------------------------------------

proc toggleEvents {topology nevents} {
    global	cnet_evname events_displayed
    global	fw fh

    set w .events
    if {[info command $w] == ""} {
# determine font-length of longest event name
	set maxword 0
	for {set n 0} {$n < $nevents} {incr n} {
	    set l [font measure ttfont _evname($n)]
	    if {$maxword < $l} {
		set maxword $l
	    }
	}
	initPopup $w "$topology events" "toggleEvents $topology $nevents" \
			[expr $maxword + 150 + 19 * $fw] \
			[expr ($nevents+3) * $fh]

	set can $w.cf.can

	set x0 [expr $maxword + 10 * $fw]
	set x1 [expr $x0 + 1]
	set x2 [expr $x0 + 150 + 6 * $fw]
	set y0 [expr $fh * 2]
	set y1 [expr $y0 - $fh/2]
	set y2 [expr $y1 + $fh]

	$can create text [expr $x0 + 150] $y0 -anchor w \
	    -font ttfont -text " #events"

	for {set n 0} {$n < $nevents} {incr n} {
	    $can create text $x0 $y0 -anchor e \
		    -font ttfont -text $cnet_evname($n)
	    $can create rectangle $x0 $y1 $x1 $y2 \
		    -fill blue -width 0 -tag evb$n
	    $can create text $x2 $y0 -anchor e \
		    -font ttfont -text "" -tag evt$n
	    incr y0 $fh
	    incr y1 $fh
	    incr y2 $fh
	}

	set y0 $fh
	set y1 [expr $y0 + ($nevents+1) * $fh]
	$can create rectangle $x0 $y0 [expr $x0 + 150] $y1

	for {set p 0} {$p <= 100} {incr p 20} {
	    $can create line $x0 $y0 $x0 $y1 -dash { 2 2 }
	    $can create text $x0 $y1 -anchor n -font ttfont -text $p%
	    incr x0 30
	}
    }
    if {[wm state $w] == "withdrawn"} {
	update idletasks
	wm deiconify $w
	raise $w
	set events_displayed true
    } else {
	wm withdraw $w
	set events_displayed false
    }
}

proc updateEvents {nevents total s} {
    set w .events.cf.can

# no need to consider 0th, EV_NULL
    for {set n 1} {$n < $nevents} {incr n} {
	set v   [lindex $s $n]
	set len [expr $v * 150 / $total ]
	scan [$w coords evb$n] "%s %s %s %s" x0 y0 x1 y1
	$w coords evb$n $x0 $y0 [expr $x0 + $len] $y1
	$w itemconfig evt$n -text [expr {$v == 0 ? "" : $v}]
    }
}

# ----------------------------------------------------------------

proc saveTopology {saveproc e} {
    $saveproc [$e get]
    wm withdraw .save
}

proc toggleSave {saveproc} {
    global	framebg canvasbg

    set w .save
    if {[info command $w] == ""} {
	newToplevel $w "save topology" "toggleSave $saveproc"
	set s [frame $w.f]
	label $s.l \
	    -background $framebg \
	    -text "Topology filename:"
	entry $s.e	-background $canvasbg -width 30
	bind $s.e <Return> "saveTopology $saveproc $s.e"
	pack $s.l $s.e -side left -fill both -expand yes
	pack $s

	button $w.cancel \
	    -background $framebg \
	    -text cancel \
	    -command "wm withdraw $w"
	button $w.save \
	    -background $framebg \
	    -text save \
	    -command "saveTopology $saveproc $s.e"
	pack $w.cancel $w.save -side left -fill both -expand yes
    }
    if {[wm state $w] == "withdrawn"} {
	update idletasks
	wm deiconify $w
    } else {
	wm withdraw $w
    }
}

# ----------------------------------------------------------------

proc toggleStats {topology nstats} {
    global	STATS_TITLES stats_displayed
    global	fw fh

    set w .stats
    if {[info command $w] == ""} {
	initPopup $w "$topology statistics" "toggleStats $topology $nstats" \
		[expr 66 * $fw] [expr ($nstats+2) * $fh]

	set can .stats.cf.can

	set x0 [expr  2 * $fw]
	set x1 [expr 44 * $fw]
	set x2 [expr 60 * $fw]
	set y  [expr  2 * $fh]
	for {set n 0} {$n < $nstats} {incr n} {
	    $can create text $x0 $y -anchor w \
		-font ttfont -text $STATS_TITLES($n)
	    $can create text $x1 $y -anchor e \
		-font ttfont -text - -tag si$n
	    $can create text $x2 $y -anchor e \
		-font ttfont -text - -tag st$n
	    incr y $fh
	}

	set x [expr 44 * $fw]
	set y $fh
	$can create text $x $y -anchor e -font ttfont -text "Instantaneous"
	set x [expr 60 * $fw]
	$can create text $x $y -anchor e -font ttfont -text "Total"

	incr x $fw
	incr y $fh
	$can create text $x $y -anchor w -font ttfont -text "usec"
	set y [expr  9 * $fh]
	$can create text $x $y -anchor w -font ttfont -text "bps"
	set y [expr  10 * $fh]
	$can create text $x $y -anchor w -font ttfont -text "usec"
	set y [expr  18 * $fh]
	$can create text $x $y -anchor w -font ttfont -text "%"
    }
    if {[wm state $w] == "withdrawn"} {
	update idletasks
	wm deiconify $w
	raise $w
	set stats_displayed true
    } else {
	wm withdraw $w
	set stats_displayed false
    }
}

proc updateStats {nstats instant total} {
    set w .stats.cf.can

    for {set n 0} {$n < $nstats} {incr n} {
	$w itemconfigure si$n -text [lindex $instant $n]
	$w itemconfigure st$n -text [lindex $total   $n]
    }
}

proc set_wifi_bars {n b} {
    set w .node$n.top.cf.stats
    for {set i 1} {$i <= $b} {incr i} {
	$w itemconfigure wifi${n}bar${i} -fill blue
    }
    for { } {$i <= 5} {incr i} {
	$w itemconfigure wifi${n}bar${i} -fill "#dddddd"
    }
}

# ----------------------------------------------------------------

proc newNode {n nodename nodetype nLEDs nwlans scalemaxs scalevalues} {

    global	fw fh

    global	stdiowin stdiorows stdiocols stdiohistory 
    global	stdioinput
    global	framebg canvasbg
    global	trace_events stdio_quiet

    set w .node$n

    if {$n == "default"} {
	set wintitle "default node attributes"
	set nn -1
	set minscale 0
    } else {
	set wintitle "$nodename"
	set nn 0
	set minscale -1
    }

    newToplevel $w "$wintitle" "toggleNode $n 0 0"
    wm resizable $w 0 0

    frame $w.top -relief flat
    frame $w.top.left -relief flat

    set f [frame $w.top.left.checks]
    checkbutton $f.trace \
	-font	myfont \
	-text	" trace events"\
	-anchor w \
	-background $framebg \
	-variable "trace_events($n)" \
	-relief flat
    checkbutton $f.quiet \
	-font	myfont \
	-text	" stdio quiet" \
	-anchor w \
	-background $framebg \
	-variable "stdio_quiet($n)" \
	-relief flat

    pack $f.trace $f.quiet -side left -fill both -expand yes
    pack $f -side top -fill both -expand yes

# add Application Layer scales to hosts, mobiles, and default
    if {$nodetype != "router" && $nodetype != "accesspoint"} {
	set b [frame $w.top.left.scales]
	foreach c {0 1 2} {
	    set b1 $b.scale$c
	    scale $b1	-from $minscale -to [lindex $scalemaxs $c] \
		-background $framebg \
		-troughcolor $canvasbg \
		-font myfont \
		-orient horizontal \
		-length 150 \
		-label "" \
		-showvalue 0 \
		-width 10 \
		-command "set_node_scale $n $c"
	    $b1 set [lindex $scalevalues $c]
	    pack $b1 -side left -fill both -expand yes
	}
	pack $b.scale0 $b.scale1 $b.scale2 \
	    -side top -fill both -expand yes
	pack $b -side top -fill both -expand yes
	pack $w.top.left.checks $w.top.left.scales \
	    -side top -fill both -expand yes
    }
    pack $w.top.left -side top -fill both -expand yes

# add a statistics canvas to nodes with Application Layers (hosts and mobiles)
    if {$nodetype == "host" || $nodetype == "mobile"} {

	newCanvas $w.top stats [expr 43 * $fw] [expr 5 * $fh]
	set x [expr 26 * $fw]
	set y $fh
	foreach s {"Messages" "Bytes" "KBytes/sec"} {
	    $w.top.cf.stats create text $x $y -anchor e -font ttfont \
			-text $s
	    incr x [expr 12 * $fw]
	}

	set x [expr 2 * $fw]
	set y [expr 2 * $fh]
	foreach s {"Generated" "Received OK" "Errors received"} {
	    $w.top.cf.stats create text $x $y -anchor w -font ttfont \
			-text $s
	    incr y $fh
	}
# add LEDs to non-default nodes
	set x 20
	set y [expr 6 * $fh]
	for {set l 0} {$l < $nLEDs} {incr l} {
	    $w.top.cf.stats create \
		rectangle $x $y [expr $x + 10] [expr $y - 10] \
		-fill $canvasbg  -tags node${n}led${l}
	    incr x 12
	}
# possibly add WiFi bars
	if {$nwlans > 0} {
	    set sq 8
	    set dx [expr $sq + 2]
	    set x0 [expr 40 * $fw]
	    set x1 [expr $x0 + $sq]
	    set y0 [expr 6 * $fh]
	    set y1 [expr $y0 - $sq]

	    $w.top.cf.stats create text $x0 [expr $y0 - 4 * $sq] \
		-font ttfont -text "WLAN" -anchor sw
	    foreach b {1 2 3 4 5} {
		$w.top.cf.stats create rect $x0 $y0 $x1 $y1 \
			-fill "#dddddd" \
			-tags wifi${n}bar${b}
		incr x0  $dx
		incr x1  $dx
		incr y1 -$dx
	    }
	}
	pack $w.top.left $w.top.cf -side left -fill both -expand yes
    }
    pack $w.top -fill both -expand yes

# add debug buttons and a stdio canvas to all nodes except the default node
    if {$n != "default"} {
	set f [frame $w.d -background $framebg]
	foreach i {0 1 2 3 4} {
	    button "$f.debug$i"	-font ttfont -text " " \
		-background $framebg \
		-command "node_debug_button $n $i"
	}
	pack $f.debug0 $f.debug1 $f.debug2 $f.debug3 $f.debug4 \
	    -side left -fill x -expand yes
	pack $f -side top -fill x -expand yes

	set win [newText $w $stdiorows $stdiocols $stdiohistory]
	set stdiowin($n)	$win
	set stdioinput($n)	""

	bind $win <Enter>    { focus %W ; break }
	bind $win <KeyPress> "stdiokey $n %K %A ; break"
    }

    set node_displayed($n) false
}

proc updateNodeStats {n s} {
    global	fw fh

    set w .node$n.top.cf.stats
    $w delete s

    for {set n 0} {$n < 8} {incr n} {
	set x [expr $fw * (26 + ($n % 3)*12)]
	set y [expr $fh * ( 2 + ($n / 3))]
	$w create text $x $y -font ttfont \
		    -anchor e \
		    -text [lindex $s $n] -tags {s}
    }
}

proc toggleNode {n x y} {
    global	node_displayed

    set w .node$n
    if {[wm state $w] == "withdrawn"} {
	incr x 20
	incr y 20
	wm geometry $w +$x+$y
	wm deiconify $w
	raise $w
	set node_displayed($n) true
    } else {
	wm withdraw $w
	set node_displayed($n) false
    }
}

proc SetDebugString {w str} {
    $w configure -text $str
}

# ----------------------------------------------------------------

proc toggleNIC {nictype from to nicname x y desc} {
    global	fw fh
    global	framebg canvasbg
    global	nic_scale_max nic_scale_value
    global	nic_displayed

    set w .nic($nictype,$from,$to)
    if {[info command $w] == ""} {

	newToplevel $w "$nicname" \
		"toggleNIC $nictype $from $to \"$nicname\" 0 0 \"$desc\""

	regsub -all "(\\+|x)" [wm geometry . ] " " geom
	set x [expr $x + [lindex $geom 2] + 30]
	set y [expr $y + [lindex $geom 3] + 90]
	wm geometry $w +$x+$y

	set t [frame $w.top]
	label $t.l -justify left \
	    -background $framebg \
	    -font myfont -text "$desc"

	pack $t.l -side left -fill both -expand yes
	pack $t -side top -fill both -expand yes

	set min  0
# an actual (not default) NIC
	if {$from != "-1"} {
	    if {$nictype == "WAN"} {
		set sh { "Frames" "Bytes" "KBytes/sec" }
		set sv { "Transmitted" "Received" "Errors Introduced" }
	    } else {
		set sh { "Packets" "Bytes" "KBytes/sec" }
		if {$to == "-1"} {
		    set sv { "Transmitted" "Received" "Total collisions" }
		} else {
		    set sv { "Transmitted" "Received" "Collisions caused" }
		}
	    }

# a new statistics canvas for actual NICs
	    newCanvas $w stats [expr 54 * $fw] [expr 5 * $fh]
	    set y $fh
	    set x [expr 30 * $fw]
	    foreach s {0 1 2} {
		$w.cf.stats create text $x $y -anchor e -font ttfont \
			    -text [lindex $sh $s]
		incr x [expr 12 * $fw]
	    }

	    set x [expr 2 * $fw]
	    set y [expr 2 * $fh]
	    foreach s {0 1 2} {
		$w.cf.stats create text $x $y -anchor w -font ttfont \
			    -text [lindex $sv $s]
		incr y $fh
	    }
	    set min -1
	}
	nic_created $nictype $from $to

# add scales for probability of frame loss & corruption, and costs
	set b [frame $w.scales]
	foreach c {0 1 2 3} {
	    set f $b.scale$c
	    scale $f	-from $min -to $nic_scale_max($c) \
		-background $framebg \
		-troughcolor $canvasbg \
		-font myfont \
		-orient horizontal \
		-length 250 \
		-label "" \
		-showvalue 0 \
		-width 10 \
		-command "set_nic_scale $nictype $from $to $c"
	    $f set $nic_scale_value($c)
	    pack $f -side left -fill both -expand yes
	}

	pack $b.scale0 $b.scale1 $b.scale2 $b.scale3 \
	    -side top -fill both -expand yes
	pack $b -side top -fill both -expand yes

	wm resizable $w 0 0
    }
    if {[wm state $w] == "withdrawn"} {
	update idletasks
	wm deiconify $w
	raise $w
	set nic_displayed($nictype,$from,$to) true
    } else {
	wm withdraw $w
	set nic_displayed($nictype,$from,$to) false
    }
}

proc updateNICstats {nictype from to s} {
    global	fw fh

    set w .nic($nictype,$from,$to)
    $w.cf.stats delete s

    for {set n 0} {$n < 8} {incr n} {
	set x [expr $fw * (30 + ($n % 3)*12)]
	set y [expr $fh * ( 2 + ($n / 3))]
	$w.cf.stats create text $x $y -anchor e -font ttfont \
		    -text [lindex $s $n] -tags {s}
    }
}

# ----------------------------------------------------------------

set nodemenu_displayed -1
# only allow one nodemenu

proc popupNodemenu {n nodename mainwin activemask x y} {
    global	nodemenu_label nodemenu_displayed

    set m .nodemenu
    if {[info command $m] == ""} {
	menu $m -tearoff false
	$m add command -label $nodename -state disabled
	$m add separator
	foreach s {0 1 2 3 4} {
	    $m add command -label $nodemenu_label($s) \
			   -command "PopdownNodeMenu $m $s"
	}
    }
    if {$nodemenu_displayed != -1} {
	set nodemenu_displayed -1
	$m unpost
	return
    }

    $m entryconfigure 0 -label $nodename
    foreach s {0 1 2 3 4} {
	$m entryconfigure [expr $s + 2] \
	    -state [expr {[expr $activemask&(1<<$s)] ? "normal" : "disabled"}]
    }
    set nodemenu_displayed $n

    regsub -all "(\\+|x)" [wm geometry $mainwin] " " geom
    set x [expr $x + [lindex $geom 2]]
    set y [expr $y + [lindex $geom 3] + 10]
    $m post $x $y
}

proc PopdownNodeMenu {m s} {
    global	nodemenu_displayed nodemenu_select

    nodemenu_select $nodemenu_displayed $s
    set nodemenu_displayed -1
    $m unpost
}

# ----------------------------------------------------------------

set nicmenu_displayed -1
# only allow one nicmenu

proc popupNICmenu {l nicname activemask x y} {
    global	nicmenu_label nicmenu_displayed

    set m .nicmenu
    if {[info command $m] == ""} {
	menu $m -tearoff false
	$m add command -label $nicname -state disabled
	$m add separator
	foreach s {0 1 2 3 4} {
	    $m add command -label $nicmenu_label($s) \
			   -command "PopdownLinkMenu $m $s"
	}
    }
    if {$nicmenu_displayed != -1} {
	set nicmenu_displayed -1
	$m unpost
	return
    }

    $m entryconfigure 0 -label $nicname
    foreach s {0 1 2 3 4} {
	$m entryconfigure [expr $s + 2] \
	    -state [expr {[expr $activemask&(1<<$s)] ? "normal" : "disabled"}]
    }
    set nicmenu_displayed $l

    regsub -all "(\\+|x)" [wm geometry .] " " geom
    set x [expr $x + [lindex $geom 2]]
    set y [expr $y + [lindex $geom 3] + 10]
    $m post $x $y
}

proc PopdownLinkMenu {m s} {
    global	nicmenu_displayed nicmenu_select

    nicmenu_select $nicmenu_displayed $s
    set nicmenu_displayed -1
    $m unpost
}

# ------------------------------------------------------------------------

proc toggleDrawframe {n title width height} {
    global drawframe_displayed

    set w .drawframe$n
    if {[info command $w] == ""} {
	newToplevel $w "$title" "toggleDrawframe $n 0 0 0"
	newCanvas $w can $width $height
	wm resizable $w 0 0
    }
    if {[wm state $w] == "withdrawn"} {
	update idletasks
	wm deiconify $w
	raise $w
	set drawframe_displayed($n) true
    } else {
	wm withdraw $w
	set drawframe_displayed($n) false
    }
}

proc DrawFrame {n nfields x y tag dir colours lengths str} {
    set c  .drawframe$n.cf.can
    set x0 $x
    set y0 [expr $y + 16]

    for {set n 0} {$n < $nfields} {incr n} {
	set dx [lindex $lengths $n]
	set x1 [expr $x + ($dir * $dx)]
	$c create rectangle $x $y $x1 $y0 \
		-fill [lindex $colours $n] -tags $tag
	set x $x1
    }
    $c create text [expr ($x0 + $x) / 2] [expr $y + 8] \
	-anchor c -font ttfont \
	-text $str -tags t$tag
}

proc SetLED {n l c} {
    global	canvasbg
    global	map

    if {"$c" == ""} {
	set c $canvasbg
    }
    .node$n.top.cf.stats itemconfigure node${n}led${l} -fill $c
    $map itemconfigure map${n}led${l} -fill $c
}

# ------------------------------------------------------------------------

proc stdioclr {n} {
    global	stdiowin stdiohistory

    set w $stdiowin($n)
    $w delete 1.0 end
    for {set l1 1} {$l1 < $stdiohistory} {incr l1} {
	$w insert $l1.0 "\n"
    }
    $w see $stdiohistory.0
}

proc stdiooutput {n str newline} {
    global	stdiowin stdiohistory

    set w $stdiowin($n)
    $w insert $stdiohistory.end "$str"
    if {$newline} then {
	for {set l1 1; set l2 2} {$l1 <= $stdiohistory} {incr l1; incr l2} {
	    $w delete $l1.0 $l1.end
	    $w insert $l1.0 [$w get $l2.0 $l2.end]
	}
    }
}

proc stdiokey {n keysym ascii} {
    global	stdiowin stdioinput stdiohistory

    set w $stdiowin($n)
    if {$keysym == "BackSpace" || $keysym == "Delete"} then {
	set len [string length $stdioinput($n)]
	if {$len == 0} then {
	    bell
	} else {
	    set s $stdioinput($n)
	    set s [string range $s 0 [expr $len - 2]]
	    set stdioinput($n) $s
	    $w delete "insert - 1 char"
	}
    } elseif {$keysym == "Return" } then {
	for {set l1 1; set l2 2} {$l1 < $stdiohistory} {incr l1; incr l2} {
	    $w delete $l1.0 $l1.end
	    $w insert $l1.0 [$w get $l2.0 $l2.end]
	}
	$w delete $stdiohistory.0 $stdiohistory.end
	stdio_input $n "$stdioinput($n)"
	set stdioinput($n)	""
    } else {
	set stdioinput($n) "$stdioinput($n)$ascii"
	$w insert $stdiohistory.end $ascii
    }
}

# ------------------------------------------------------------------------

set exitplease 0

proc show_error {progname msg filenm errline} {
    global	framebg
    global	stdiorows stdiocols
    global	exitplease

    newToplevel .error "runtime error" "set exitplease 1"

    set f [frame .error.f -background $framebg]
    label $f.l -justify left \
		-background $framebg \
		-font ttfont -text "$msg"
    pack $f.l
    pack $f -side top -fill x

    if {$filenm != ""} {
        set cmd {
            set fid [open $filenm "r"]
        }
        if {[catch $cmd result] == 0} {
	    set w [frame .error.code]
	    set t [newText $w $stdiorows $stdiocols 0]
	    $t configure -state normal

	    set lc 0
	    while {1} {
		set line [gets $fid]
		if {[eof $fid]} {break}
		incr lc
		if {$lc == $errline} {
		    $t insert end [format "%4d\t%s\n" $lc $line] bg2
		} elseif {[expr $lc % 2] == 0} {
		    $t insert end [format "%4d\t%s\n" $lc $line] bg1
		} else {
		    $t insert end "\t$line\n"
		}
	    }
            close $fid
	    $t configure -state disabled
	    pack $w -side top -fill both -expand yes

	    set half [expr $errline - [expr $stdiorows / 2 ] ]
	    $t see $half.0
	    bind $t <ButtonPress> "$t see $half.0"
        }
    }
    update idletasks
    wm deiconify .error
    raise .error
    grab set .error
    vwait exitplease
    grab release .error
}
