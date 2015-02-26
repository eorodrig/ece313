#if	HIDE_GLOBAL_NAMES
#define	CLEANUP                             	__CN001
#define	DEFAULTLAN                          	__CN002
#define	DEFAULTWAN                          	__CN003
#define	DEFAULTWLAN                         	__CN004
#define	DEF_NODEATTR                        	__CN005
#define	ERROR                               	__CN006
#define	FATAL                               	__CN007
#define	HANDLING                            	__CN008
#define	LANS                                	__CN009
#define	MICROSECONDS                        	__CN010
#define	NICdesc                             	__CN011
#define	NODES                               	__CN012
#define	NP                                  	__CN013
#define	REPORT                              	__CN014
#define	TCLTK_init                          	__CN015
#define	THISNODE                            	__CN016
#define	TRACE                               	__CN017
#define	WANS                                	__CN018
#define	WARNING                             	__CN019
#define	WLANS                               	__CN020
#define	Wflag                               	__CN021
#define	add_drawframe                       	__CN022
#define	add_lan                             	__CN023
#define	add_linktype                        	__CN024
#define	add_node                            	__CN025
#define	add_nodetype                        	__CN026
#define	add_wan                             	__CN027
#define	add_wlan                            	__CN028
#define	argv0                               	__CN029
#define	assign_nicaddr                      	__CN030
#define	chararray                           	__CN031
#define	check_application_bounds            	__CN032
#define	check_lans                          	__CN033
#define	check_topology                      	__CN034
#define	check_wans                          	__CN035
#define	check_wlans                         	__CN036
#define	clone_mobile_node                   	__CN037
#define	clone_node_vars                     	__CN038
#define	cnet_colours                        	__CN039
#define	cnet_state                          	__CN040
#define	compile4ipod                        	__CN041
#define	compile_string                      	__CN042
#define	compile_topology                    	__CN043
#define	corrupt_frame                       	__CN044
#define	debug_pressed                       	__CN045
#define	dflag                               	__CN046
#define	display_nodemenu                    	__CN047
#define	display_wanmenu                     	__CN048
#define	draw_lans                           	__CN049
#define	draw_node_icon                      	__CN050
#define	draw_wan                            	__CN051
#define	draw_wans                           	__CN052
#define	draw_wlansignal                     	__CN053
#define	extend_lan                          	__CN054
#define	find_cnetfile                       	__CN055
#define	find_linktype                       	__CN056
#define	find_mapsize                        	__CN057
#define	find_nodetype                       	__CN058
#define	find_nspeeds                        	__CN059
#define	find_trace_name                     	__CN060
#define	findenv                             	__CN061
#define	flush_GUIstats                      	__CN062
#define	flush_allstats                      	__CN063
#define	flush_wlan_history                  	__CN064
#define	forget_drawframes                   	__CN065
#define	format_nodeinfo                     	__CN066
#define	gattr                               	__CN067
#define	get_next_event                      	__CN068
#define	gettoken                            	__CN069
#define	init_application_layer              	__CN070
#define	init_defaultlan                     	__CN071
#define	init_defaultwan                     	__CN072
#define	init_defaultwlan                    	__CN073
#define	init_drawframes                     	__CN074
#define	init_eventswindow                   	__CN075
#define	init_globals                        	__CN076
#define	init_lans                           	__CN077
#define	init_lexical                        	__CN078
#define	init_mainwindow                     	__CN079
#define	init_nicattrs                       	__CN080
#define	init_nodeattrs                      	__CN081
#define	init_nodewindow                     	__CN082
#define	init_physical_layer                 	__CN083
#define	init_queuing                        	__CN084
#define	init_reboot_argv                    	__CN085
#define	init_scheduler                      	__CN086
#define	init_stats_layer                    	__CN087
#define	init_statswindows                   	__CN088
#define	init_stdio_layer                    	__CN089
#define	init_trace                          	__CN090
#define	init_wans                           	__CN091
#define	init_wlans                          	__CN092
#define	input                               	__CN093
#define	internal_event                      	__CN094
#define	invoke_shutdown                     	__CN095
#define	lose_frame                          	__CN096
#define	motd                                	__CN097
#define	mouse_position                      	__CN098
#define	move_drawframe                      	__CN099
#define	mt19937_init                        	__CN100
#define	mt19937_int31                       	__CN101
#define	mt19937_int32                       	__CN102
#define	mt19937_real53                      	__CN103
#define	nerrors                             	__CN104
#define	new_drawframe                       	__CN105
#define	newevent                            	__CN106
#define	nextch                              	__CN107
#define	nic_created                         	__CN108
#define	node_click                          	__CN109
#define	nqueued                             	__CN110
#define	parse_topology                      	__CN111
#define	poisson_usecs                       	__CN112
#define	poll_application                    	__CN113
#define	prepare4framearrival                	__CN114
#define	prepare4framecollision              	__CN115
#define	print_lans                          	__CN116
#define	print_linktypes                     	__CN117
#define	print_nic1                          	__CN118
#define	print_node_attr                     	__CN119
#define	print_nodetypes                     	__CN120
#define	qflag                               	__CN121
#define	random_topology                     	__CN122
#define	reboot_application_layer            	__CN123
#define	reboot_stdio_layer                  	__CN124
#define	reboot_timer_layer                  	__CN125
#define	report_linkstate                    	__CN126
#define	save_topology                       	__CN127
#define	schedule                            	__CN128
#define	schedule_application                	__CN129
#define	schedule_drawwlansignal             	__CN130
#define	schedule_lanbusy                    	__CN131
#define	schedule_moveframe                  	__CN132
#define	set_keyboard_input                  	__CN133
#define	set_lanbusy                         	__CN134
#define	set_node_var                        	__CN135
#define	single_event                        	__CN136
#define	std_CNET_disable_application        	__CN137
#define	std_CNET_enable_application         	__CN138
#define	std_CNET_read_application           	__CN139
#define	std_CNET_write_application          	__CN140
#define	std_application_bounds              	__CN141
#define	std_init_application_layer          	__CN142
#define	std_poll_application                	__CN143
#define	std_reboot_application_layer        	__CN144
#define	tcl_interp                          	__CN145
#define	tcltk_notify_dispatch               	__CN146
#define	tcltk_notify_start                  	__CN147
#define	tcltk_notify_stop                   	__CN148
#define	tk_stdio_input                      	__CN149
#define	toggle_drawframes                   	__CN150
#define	toggle_drawwlans                    	__CN151
#define	token                               	__CN152
#define	trace_handler                       	__CN153
#define	unschedule_application              	__CN154
#define	unschedule_lan_collision            	__CN155
#define	unschedule_link                     	__CN156
#define	unschedule_node                     	__CN157
#define	unschedule_timer                    	__CN158
#define	vflag                               	__CN159
#define	write_lan                           	__CN160
#define	write_wan                           	__CN161
#define	write_wlan                          	__CN162
#endif
