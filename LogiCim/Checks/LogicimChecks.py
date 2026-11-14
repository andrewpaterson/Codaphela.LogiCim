#!/usr/bin/env python3
#LogiCim's checks: LogiCim is run with LOGICIM_CHECK set to steps (see CLogicimApp::Check), which it runs once its
#window is set up, without showing it, and what it prints is checked.
#
#  LogicimChecks.py [logicim executable] [check name...]
#
#The executable defaults to ../../../build/LogiCimRelease/LogiCim.  Exits non-zero if a check fails.
import os, re, shutil, subprocess, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
LOGICIM_DIR = os.path.join(HERE, '..', 'LogiCim')
ROOT = os.path.abspath(os.path.join(HERE, '..', '..', '..'))

#Each check: its name, its LOGICIM_CHECK steps, the lines (regular expressions) its output must have, in order, and
#lines it mustn't have.
CHECKS = [
	('commands', 'commands',
		[r'^Commands: [1-9][0-9]*$',	#ELogicimAction's actions, the commands only keys run, and a Place command for each library component no action places.
		 r'^Command: File\.LoadProject$',
		 r'^Command: Edit\.FlipHorizontally \[Control\+Shift\+F, H\] bound$',
		 r'^Command: Edit\.FlipVertically \[Control\+Shift\+F, V\] bound$',
		 r'^Command: File\.Exit$',
		 r'^Command: Edit\.StepUndo \[Control\+Alt\+Z\] bound$',
		 r'^Command: Edit\.StepRedo \[Control\+Alt\+Shift\+Z\] bound$',
		 r'^Command: Circuit\.LeaveSubcircuit \[Alt\+Backspace\] bound$',
		 r'^Command: File\.Settings \[Control\+Alt\+S\] bound$'],
		[r'NOT BOUND']),
	('menus-and-buttons', 'menus,buttons',
		[r'^Menu File: New Project \[File\.NewProject\] Load Project \[File\.LoadProject\] Save Project \[File\.SaveProject\] Recent Projects \[none\] Clear Recent Projects \[File\.ClearRecentProjects\] Import KiCad Schematic\.\.\. \[File\.ImportKicadSchematic\] Settings\.\.\. \[File\.Settings\] Exit \[File\.Exit\]$',
		 r'^Menu Edit: Undo \[Edit\.Undo\] Redo \[Edit\.Redo\] ',
		 r'^Menu View: .* Subcircuits \[View\.Subcircuits\] Simulations \[View\.Simulations\]$',
		 r'^Menu Simulation: ',
		 r'^Menu Place: ',
		 r'^Menu Window: Reset Window Layout \[Window\.ResetLayout\]$',
		 r'^Buttons: [1-9][0-9]* with a command, 0 without$'],
		[r'(?<!Recent Projects )\[none\]']),	#Recent Projects opens its submenu; every other item runs a command.
	('run-commands', 'zoom,command:View.ZoomIn,zoom,command:View.ZoomOut,command:View.ZoomOut,zoom,command:View.ResetZoom,zoom',
		[r'^Zoom: 1\.00$', r'^Command View\.ZoomIn: run$', r'^Zoom: 2\.00$', r'^Command View\.ZoomOut: run$',
		 r'^Command View\.ZoomOut: run$', r'^Zoom: (0\.\d\d|1\.00)$', r'^Command View\.ResetZoom: run$', r'^Zoom: 1\.00$'],
		[r'Unknown check step', r'not run']),
	#Over the circuit bare and shifted letters are its keys (Logicim's KeyInputsFactory's), and the Global ones still work.
	('circuit-keys', 'commands,keys:Circuit:R,keys:Circuit:Shift+R,keys:Circuit:Shift+A,keys:Circuit:P,keys:Circuit:M,keys:Circuit:Control+Alt+S,keys:Global:R,command:Place.AndGate',
		[r'^Command: Edit\.RotateLeft \[R \(Circuit\)\] bound$',
		 r'^Command: Edit\.FlipHorizontally \[Control\+Shift\+F, H\] bound$',
		 r'^Command: Place\.Power \[V \(Circuit\)\] bound$',
		 r'^Command: Edit\.Move \[M \(Circuit\)\] bound$',
		 r'^Command: Place\.AndGate \[Shift\+A \(Circuit\)\] bound$',
		 r'^Command: Place\.Logic74Package:00$',
		 r'^Keys R in Circuit: Edit\.RotateLeft$', r'^Keys Shift\+R in Circuit: Edit\.RotateRight$',
		 r'^Keys Shift\+A in Circuit: Place\.AndGate$', r'^Keys P in Circuit: Place\.Pin$', r'^Keys M in Circuit: Edit\.Move$',
		 r'^Keys Control\+Alt\+S in Circuit: File\.Settings$', r'^Keys R in Global: none$',
		 r'^Command Place\.AndGate: run$'],
		[r'Unknown check step', r'NOT BOUND', r'not run']),
]


def Run(szExecutable, szSteps, asExtraEnv=None):
	asEnv = dict(os.environ)
	asEnv.pop('LOGICIM_SETTINGS', None)
	asEnv.pop('LOGICIM_RECENT', None)
	asEnv.pop('LOGICIM_LAYOUT', None)
	asEnv.pop('LOGICIM_KEYMAP', None)
	asEnv.pop('LOGICIM_KEYMAP_EDITS', None)
	asEnv.pop('LOGICIM_CHOOSER_EDITS', None)
	asEnv['LOGICIM_CHECK'] = szSteps
	if asExtraEnv:
		asEnv.update(asExtraEnv)
	cResult = subprocess.run([szExecutable], cwd=LOGICIM_DIR, env=asEnv, capture_output=True, timeout=120)
	szOut = cResult.stdout.decode('latin-1') + cResult.stderr.decode('latin-1')
	return cResult.returncode, szOut.split('\n')


def Check(szExecutable, szName, szSteps, aszExpected, aszForbidden, asExtraEnv=None):
	iCode, aszLines = Run(szExecutable, szSteps, asExtraEnv)
	aszProblems = []
	if iCode != 0:
		aszProblems.append('exited with %d' % iCode)
	iLine = 0
	for szExpected in aszExpected:
		while (iLine < len(aszLines)) and not re.search(szExpected, aszLines[iLine]):
			iLine += 1
		if iLine == len(aszLines):
			aszProblems.append('no line (in order) matching: ' + szExpected)
			break
		iLine += 1
	if os.environ.get('LOGICIM_VERIFY_REDRAW'):
		#Every partial paint is checked against drawing all of the window; one that differed is logged.
		aszForbidden = aszForbidden + [r'A partial paint of']
	for szForbidden in aszForbidden:
		for szLine in aszLines:
			if re.search(szForbidden, szLine):
				aszProblems.append('forbidden line: ' + szLine)
	if aszProblems:
		print('FAIL ' + szName)
		for szProblem in aszProblems:
			print('    ' + szProblem)
		print('    output:')
		for szLine in aszLines:
			print('    | ' + szLine)
		return False
	print('pass ' + szName)
	return True


#Import KiCad Schematic and Load Project each open where they were last used, kept apart and kept between runs in
#LOGICIM_SETTINGS; Save Project offers the project's own file.  A scripted run without LOGICIM_SETTINGS keeps nothing.
def CheckRememberedFiles(szExecutable):
	szDirectory = tempfile.mkdtemp(prefix='LogicimChecks')
	try:
		szSettings = os.path.join(szDirectory, 'Settings.xml')
		szProjects = os.path.join(szDirectory, 'Projects')
		szKicad = os.path.join(szDirectory, 'Kicad')
		os.makedirs(szProjects)
		os.makedirs(szKicad)
		szProject = os.path.join(szProjects, 'FlipFlop.xml')
		shutil.copy(os.path.join(LOGICIM_DIR, 'FlipFlop.xml'), szProject)
		szSchematic = os.path.join(szKicad, 'Empty.kicad_sch')
		with open(szSchematic, 'w') as cFile:
			cFile.write('(kicad_sch (version 20250114) (generator "eeschema") (uuid "root") (paper "A4"))\n')
		szImported = os.path.join(szKicad, 'Empty.xml')
		szSaved = os.path.join(szProjects, 'Saved.xml')
		szMissing = os.path.join(szProjects, 'Missing.xml')
		asEnv = {'LOGICIM_SETTINGS': szSettings}
		E = re.escape

		bPassed = Check(szExecutable, 'remembered-files', ','.join(['files', 'load:' + szProject, 'files', 'import:' + szSchematic, 'files', 'save:' + szSaved, 'files', 'load:' + szMissing, 'files']),
			[r'^Project file: \[\]$', r'^Load Project opens: \[Project\.xml\]$', r'^Import KiCad Schematic opens: \[Schematic\.kicad_sch\]$',
			 r'loaded$',
			 r'^Project file: \[%s\]$' % E(szProject), r'^Load Project opens: \[%s\]$' % E(szProject), r'^Save Project opens: \[%s\]$' % E(szProject),
			 r'imported$',
			 r'^Project file: \[%s\]$' % E(szImported), r'^Load Project opens: \[%s\]$' % E(szProject), r'^Save Project opens: \[%s\]$' % E(szImported), r'^Import KiCad Schematic opens: \[%s\]$' % E(szSchematic),
			 r'saved$',
			 r'^Project file: \[%s\]$' % E(szSaved), r'^Load Project opens: \[%s\]$' % E(szProject),
			 r'not loaded$',
			 r'^Project file: \[%s\]$' % E(szSaved), r'^Load Project opens: \[%s\]$' % E(szProject)],
			[r'Unknown check step'], asEnv)
		bPassed = Check(szExecutable, 'remembered-files-read', 'files',
			[r'^Project file: \[\]$', r'^Load Project opens: \[%s\]$' % E(szProject), r'^Import KiCad Schematic opens: \[%s\]$' % E(szSchematic)],
			[], asEnv) and bPassed
		os.remove(szSettings)
		bPassed = Check(szExecutable, 'remembered-files-scripted', 'load:' + szProject + ',files',
			[r'loaded$', r'^Load Project opens: \[%s\]$' % E(szProject)],
			[], {'HOME': szDirectory, 'APPDATA': szDirectory}) and bPassed
		if os.path.exists(szSettings) or os.path.exists(os.path.join(szDirectory, '.config')) or os.path.exists(os.path.join(szDirectory, 'LogiCim')):
			print('FAIL remembered-files-scripted\n    a settings file was written')
			bPassed = False
		return bPassed
	finally:
		shutil.rmtree(szDirectory, ignore_errors=True)


#File, Recent Projects: the projects loaded or saved, the latest first, each once, kept between runs in LOGICIM_RECENT.
#One chosen is loaded; one that isn't there any more is taken off; Clear Recent Projects leaves none.
def CheckRecentProjects(szExecutable):
	szDirectory = tempfile.mkdtemp(prefix='LogicimChecks')
	try:
		szRecent = os.path.join(szDirectory, 'RecentProjects.xml')
		szProject = os.path.join(szDirectory, 'FlipFlop.xml')
		szGone = os.path.join(szDirectory, 'Gone.xml')
		szSaved = os.path.join(szDirectory, 'Saved.xml')
		shutil.copy(os.path.join(LOGICIM_DIR, 'FlipFlop.xml'), szProject)
		shutil.copy(os.path.join(LOGICIM_DIR, 'FlipFlop.xml'), szGone)
		asEnv = {'LOGICIM_RECENT': szRecent, 'LOGICIM_SETTINGS': os.path.join(szDirectory, 'Settings.xml')}
		E = re.escape

		bPassed = Check(szExecutable, 'recent-projects', ','.join(['recent', 'load:' + szGone, 'load:' + szProject, 'save:' + szSaved, 'load:' + szProject, 'recent']),
			[r'^Recent Projects: 1$', r'^Recent: \[\(No recent projects\)\] disabled$',
			 r'^Recent Projects: 3$', r'^Recent: \[1 %s\]$' % E(szProject), r'^Recent: \[2 %s\]$' % E(szSaved), r'^Recent: \[3 %s\]$' % E(szGone)],
			[r'Unknown check step'], asEnv)
		os.remove(szGone)
		bPassed = Check(szExecutable, 'recent-projects-read', 'recent,openrecent:3,recent,openrecent:2,files,recent,clearrecent,recent',
			[r'^Recent Projects: 3$', r'^Recent: \[3 %s\]$' % E(szGone),
			 r'^Open recent 3: not loaded$', r'^Recent Projects: 2$', r'^Recent: \[2 %s\]$' % E(szSaved),
			 r'^Open recent 2: loaded$', r'^Project file: \[%s\]$' % E(szSaved),
			 r'^Recent Projects: 2$', r'^Recent: \[1 %s\]$' % E(szSaved), r'^Recent: \[2 %s\]$' % E(szProject),
			 r'^Recent projects cleared$', r'^Recent Projects: 1$', r'^Recent: \[\(No recent projects\)\] disabled$'],
			[r'Unknown check step'], asEnv) and bPassed
		bPassed = Check(szExecutable, 'recent-projects-cleared', 'recent',
			[r'^Recent Projects: 1$', r'^Recent: \[\(No recent projects\)\] disabled$'],
			[], asEnv) and bPassed
		return bPassed
	finally:
		shutil.rmtree(szDirectory, ignore_errors=True)


#File, New Project: the project loaded is closed and an empty "Main" circuit, without a file, edited in its place;
#Load Project still opens where it did and the recent projects stay.  Undo goes back to the old project.
def CheckNewProject(szExecutable):
	szDirectory = tempfile.mkdtemp(prefix='LogicimChecks')
	try:
		szProject = os.path.join(szDirectory, 'FlipFlop.xml')
		szEmpty = os.path.join(szDirectory, 'Empty.xml')
		szUndone = os.path.join(szDirectory, 'Undone.xml')
		shutil.copy(os.path.join(LOGICIM_DIR, 'FlipFlop.xml'), szProject)
		asEnv = {'LOGICIM_RECENT': os.path.join(szDirectory, 'RecentProjects.xml'), 'LOGICIM_SETTINGS': os.path.join(szDirectory, 'Settings.xml')}
		E = re.escape

		bPassed = Check(szExecutable, 'new-project', ','.join(['load:' + szProject, 'new', 'files', 'recent', 'save:' + szEmpty, 'command:Edit.Undo', 'save:' + szUndone]),
			[r'loaded$', r'^New project: made$',
			 r'^Project file: \[\]$', r'^Load Project opens: \[%s\]$' % E(szProject),
			 r'^Recent Projects: 1$', r'^Recent: \[1 %s\]$' % E(szProject),
			 r'saved$', r'^Command Edit\.Undo: run$', r'saved$'],
			[r'Unknown check step'], asEnv)
		aszProblems = []
		if os.path.exists(szEmpty):
			szText = open(szEmpty).read()
			if (re.findall(r'<Circuit ', szText) != ['<Circuit ']) or ('<Circuit type="Main"/>' not in szText):
				aszProblems.append('the new project is not one empty Main circuit:\n' + szText)
		else:
			aszProblems.append('the new project was not saved')
		if not os.path.exists(szUndone) or (open(szUndone).read().count('<Component') != open(szProject).read().count('<Component')):
			aszProblems.append('Undo did not go back to the project loaded')
		if aszProblems:
			print('FAIL new-project-saved')
			for szProblem in aszProblems:
				print('    ' + szProblem)
			return False
		print('pass new-project-saved')
		return bPassed
	finally:
		shutil.rmtree(szDirectory, ignore_errors=True)


#Whether the project has changes (which closing it asks to save) and its file follow undo and redo: an edit changes it,
#undoing back to where it was loaded or saved leaves it unchanged, and undoing back past a project's start (a load or
#New Project) goes back to the project before it with its file.
def CheckProjectChanges(szExecutable):
	szDirectory = tempfile.mkdtemp(prefix='LogicimChecks')
	try:
		szA = os.path.join(szDirectory, 'A.xml')
		szB = os.path.join(szDirectory, 'B.xml')
		szSaved = os.path.join(szDirectory, 'A2.xml')
		shutil.copy(os.path.join(LOGICIM_DIR, 'FlipFlop.xml'), szA)		#Loads with a component selected.
		shutil.copy(os.path.join(LOGICIM_DIR, 'FlipFlop.xml'), szB)
		asEnv = {'LOGICIM_RECENT': os.path.join(szDirectory, 'RecentProjects.xml'), 'LOGICIM_SETTINGS': os.path.join(szDirectory, 'Settings.xml')}
		E = re.escape

		bPassed = Check(szExecutable, 'project-changes', ','.join(['modified', 'load:' + szA, 'modified', 'command:Edit.Delete', 'modified',
			'command:Edit.Undo', 'modified', 'command:Edit.Redo', 'modified', 'save:' + szSaved, 'modified', 'command:Edit.Undo', 'modified',
			'command:Edit.Redo', 'modified', 'new', 'modified']),
			[r'^Modified: no$', r'loaded$', r'^Modified: no$', r'^Command Edit\.Delete: run$', r'^Modified: yes$',
			 r'^Command Edit\.Undo: run$', r'^Modified: no$', r'^Command Edit\.Redo: run$', r'^Modified: yes$',
			 r'saved$', r'^Modified: no$', r'^Command Edit\.Undo: run$', r'^Modified: yes$', r'^Command Edit\.Redo: run$', r'^Modified: no$',
			 r'^New project: made$', r'^Modified: no$'],
			[r'Unknown check step', r'not run'], asEnv)
		bPassed = Check(szExecutable, 'project-close', ','.join(['close', 'load:' + szA, 'close', 'command:Edit.Delete', 'close',
			'command:Edit.Undo', 'close']),
			[r'^Close: closes$', r'loaded$', r'^Close: closes$', r'^Command Edit\.Delete: run$', r'^Close: asks to save$',
			 r'^Command Edit\.Undo: run$', r'^Close: closes$'],
			[r'Unknown check step', r'not run'], asEnv) and bPassed
		bPassed = Check(szExecutable, 'project-undo-file', ','.join(['load:' + szA, 'save:' + szSaved, 'load:' + szB, 'files',
			'command:Edit.Undo', 'files', 'modified', 'command:Edit.Redo', 'files', 'new', 'files', 'command:Edit.Undo', 'files', 'command:Edit.Redo', 'files']),
			[r'^Project file: \[%s\]$' % E(szB),
			 r'^Command Edit\.Undo: run$', r'^Project file: \[%s\]$' % E(szSaved), r'^Save Project opens: \[%s\]$' % E(szSaved), r'^Modified: no$',
			 r'^Command Edit\.Redo: run$', r'^Project file: \[%s\]$' % E(szB),
			 r'^New project: made$', r'^Project file: \[\]$',
			 r'^Command Edit\.Undo: run$', r'^Project file: \[%s\]$' % E(szB),
			 r'^Command Edit\.Redo: run$', r'^Project file: \[\]$'],
			[r'Unknown check step', r'not run'], asEnv) and bPassed
		return bPassed
	finally:
		shutil.rmtree(szDirectory, ignore_errors=True)


#The tool windows are dock panes around the circuit, the one document; their layout is kept in LOGICIM_LAYOUT and read
#back the next time, and Reset Window Layout puts them back.  A scripted run without LOGICIM_LAYOUT keeps none.
def CheckDockLayout(szExecutable):
	szDirectory = tempfile.mkdtemp(prefix='LogicimChecks')
	try:
		szLayout = os.path.join(szDirectory, 'Layout.xml')
		asEnv = {'LOGICIM_LAYOUT': szLayout, 'LOGICIM_SETTINGS': os.path.join(szDirectory, 'Settings.xml'), 'LOGICIM_RECENT': os.path.join(szDirectory, 'RecentProjects.xml')}
		bPassed = Check(szExecutable, 'dock-layout', 'panes,hide:Simulations,panes,savelayout',
			[r'^Panes: 3$', r'^Pane Circuit \[Main\]: shown$', r'^Pane Subcircuits \[Subcircuits\]: shown$', r'^Pane Simulations \[Simulations\]: shown$',
			 r'^Hid Simulations$', r'^Pane Simulations \[Simulations\]: hidden$', r'^Layout: saved$'],
			[r'Unknown check step', r'No pane'], asEnv)
		if not os.path.exists(szLayout):
			print('FAIL dock-layout\n    no layout was written')
			bPassed = False
		bPassed = Check(szExecutable, 'dock-layout-read', 'panes,resetlayout,panes',
			[r'^Pane Subcircuits \[Subcircuits\]: shown$', r'^Pane Simulations \[Simulations\]: hidden$',
			 r'^Layout reset$', r'^Pane Simulations \[Simulations\]: shown$'],
			[r'Unknown check step'], asEnv) and bPassed
		if os.path.exists(szLayout):
			os.remove(szLayout)
		bPassed = Check(szExecutable, 'dock-layout-scripted', 'hide:Subcircuits,savelayout',
			[r'^Hid Subcircuits$', r'^Layout: saved$'],
			[], {'HOME': szDirectory, 'APPDATA': szDirectory}) and bPassed
		if os.path.exists(szLayout) or os.path.exists(os.path.join(szDirectory, '.config')) or os.path.exists(os.path.join(szDirectory, 'LogiCim')):
			print('FAIL dock-layout-scripted\n    a layout was written')
			bPassed = False
		return bPassed
	finally:
		shutil.rmtree(szDirectory, ignore_errors=True)


#File, Settings' Keymap: a key given to a command is bound and kept in LOGICIM_KEYMAP, read back the next time, and a
#key moved from another command leaves it.  A scripted run without LOGICIM_KEYMAP keeps none.
def CheckKeymap(szExecutable):
	szDirectory = tempfile.mkdtemp(prefix='LogicimChecks')
	try:
		szKeymap = os.path.join(szDirectory, 'Keymap.xml')
		asEnv = {'LOGICIM_KEYMAP': szKeymap, 'LOGICIM_SETTINGS': os.path.join(szDirectory, 'Settings.xml'), 'LOGICIM_RECENT': os.path.join(szDirectory, 'RecentProjects.xml'),
				 'LOGICIM_KEYMAP_EDITS': 'select:View.ZoomIn;add:Control+Alt+I;select:View.ZoomOut;add:Control+Alt+S'}
		bPassed = Check(szExecutable, 'keymap-edit', 'keymap,commands',
			[r'^Keymap: Ctrl\+Alt\+I is free\.$', r'^Keymap: kept$',
			 r'^Command: View\.ZoomIn \[Control\+Alt\+I\] bound$', r'^Command: View\.ZoomOut \[Control\+Alt\+S\] bound$'],
			[r'Unknown check step', r'^Command: File\.Settings \[Control\+Alt\+S\]'], asEnv)
		if not os.path.exists(szKeymap):
			print('FAIL keymap-edit\n    no keymap was written')
			bPassed = False
		del asEnv['LOGICIM_KEYMAP_EDITS']
		bPassed = Check(szExecutable, 'keymap-read', 'commands',
			[r'^Command: View\.ZoomIn \[Control\+Alt\+I\] bound$', r'^Command: View\.ZoomOut \[Control\+Alt\+S\] bound$', r'^Command: File\.Settings$'],
			[], asEnv) and bPassed
		os.remove(szKeymap)
		bPassed = Check(szExecutable, 'keymap-scripted', 'keymap,commands',
			[r'^Keymap: kept$', r'^Command: View\.ZoomIn \[Control\+Alt\+I\] bound$'],
			[], {'HOME': szDirectory, 'APPDATA': szDirectory, 'LOGICIM_KEYMAP_EDITS': 'select:View.ZoomIn;add:Control+Alt+I'}) and bPassed
		if os.path.exists(szKeymap) or os.path.exists(os.path.join(szDirectory, '.config')) or os.path.exists(os.path.join(szDirectory, 'LogiCim')):
			print('FAIL keymap-scripted\n    a keymap was written')
			bPassed = False

		#Settings' Keymap doesn't list the components' commands, their keys being the chooser's, but a key given there that
		#one has still moves from it.
		bPassed = Check(szExecutable, 'keymap-unlisted', 'keymap,keys:Circuit:Shift+A',
			[r'^Keymap row: Edit\.RotateLeft ', r'^Keymap row: Place\.Splitter ', r'^Keymap: Shift\+A moves here from Place: Add AND Gate\.$',
			 r'^Keymap: kept$', r'^Keys Shift\+A in Circuit: Edit\.Move$'],
			[r'^Keymap row: Place\.AndGate', r'^Keymap row: Place\.Logic74', r'Unknown check step'],
			{'HOME': szDirectory, 'APPDATA': szDirectory, 'LOGICIM_KEYMAP_EDITS': 'show;select:Edit.Move;add:Circuit:Shift+A'}) and bPassed

		#The component chooser's shortcut: the selected component's Place command's keys, given as Settings' Keymap gives
		#them, bound and kept when it closes, and read back the next time.
		asEnv = {'LOGICIM_KEYMAP': szKeymap, 'LOGICIM_SETTINGS': os.path.join(szDirectory, 'Settings.xml'), 'LOGICIM_RECENT': os.path.join(szDirectory, 'RecentProjects.xml'),
				 'LOGICIM_CHOOSER_EDITS': 'select:AndGate;show;add:Shift+O;show;select:OrGate;show;select:Adder;add:Control+Alt+D;show;add:Global:Shift+Q;add:Global:Control+Alt+S;select:Splitter;remove;show'}
		bPassed = Check(szExecutable, 'chooser-keys', 'chooser,commands',
			[r'^Chooser: AndGate Shortcut: Shift\+A \(Circuit\)$', r'^Chooser: Shift\+O moves here from Add OR Gate\.$',
			 r'^Chooser: AndGate Shortcut: Shift\+A \(Circuit\), Shift\+O \(Circuit\)$', r'^Chooser: OrGate Shortcut: none$',
			 r'^Chooser: Ctrl\+Alt\+D is free\.$', r'^Chooser: Adder Shortcut: Ctrl\+Alt\+D \(Circuit\)$',
			 r'^Chooser: Shift\+Q types a character: add Ctrl or Alt\.$', r'^Chooser: Ctrl\+Alt\+S moves here from Settings\.$',
			 r'^Chooser: Splitter Shortcut: none$', r'^Chooser: closed$',
			 r'^Command: Place\.Splitter$', r'^Command: File\.Settings$', r'^Command: Place\.AndGate \[Shift\+A \(Circuit\)\] bound$',
			 r'^Command: Place\.OrGate$', r'^Command: Place\.Adder \[Control\+Alt\+S\] bound$'],
			[r'Unknown check step', r'NOT BOUND'], asEnv) and bPassed
		del asEnv['LOGICIM_CHOOSER_EDITS']
		bPassed = Check(szExecutable, 'chooser-keys-read', 'keys:Circuit:Shift+O,keys:Circuit:Control+Alt+D,keys:Circuit:Control+Alt+S,keys:Circuit:Shift+S',
			[r'^Keys Shift\+O in Circuit: Place\.AndGate$', r'^Keys Control\+Alt\+D in Circuit: Place\.Adder$',
			 r'^Keys Control\+Alt\+S in Circuit: Place\.Adder$', r'^Keys Shift\+S in Circuit: none$'],
			[], asEnv) and bPassed
		return bPassed
	finally:
		shutil.rmtree(szDirectory, ignore_errors=True)


#A subcircuit's placing keys are the project's: read with it (one another command has is dropped without saying), given
#in the component chooser as a change that can be undone, and saved with it.  The keymap doesn't keep them.
def CheckSubcircuitKeys(szExecutable):
	szDirectory = tempfile.mkdtemp(prefix='LogicimChecks')
	try:
		szProject = os.path.join(szDirectory, 'Project.xml')
		szSaved = os.path.join(szDirectory, 'Saved.xml')
		szKeymap = os.path.join(szDirectory, 'Keymap.xml')
		with open(szProject, 'w') as cFile:
			cFile.write('<?xml version="1.0" encoding="UTF-8"?>\n<Logicim version="2">\n  <Editor circuit="Main" rotation="East"/>\n  <Circuits>\n'
						'    <Circuit type="Main"/>\n    <Circuit type="Half Adder" placeKeys="Circuit:Shift+H;Circuit:Shift+A;Global:Control+Alt+H"/>\n'
						'  </Circuits>\n</Logicim>\n')
		asEnv = {'LOGICIM_KEYMAP': szKeymap, 'LOGICIM_SETTINGS': os.path.join(szDirectory, 'Settings.xml'), 'LOGICIM_RECENT': os.path.join(szDirectory, 'RecentProjects.xml'),
				 'LOGICIM_CHOOSER_EDITS': 'select:Half Adder;show;add:Shift+J;show'}
		bPassed = Check(szExecutable, 'subcircuit-keys', ','.join(['load:' + szProject, 'keys:Circuit:Shift+H', 'keys:Circuit:Shift+A', 'keys:Global:Control+Alt+H', 'modified',
																	'chooser', 'modified', 'keys:Circuit:Shift+J', 'command:Edit.Undo', 'keys:Circuit:Shift+J',
																	'command:Edit.Redo', 'save:' + szSaved, 'commands']),
			[r'^Keys Shift\+H in Circuit: Place\.Subcircuit\.Half Adder$', r'^Keys Shift\+A in Circuit: Place\.AndGate$',
			 r'^Keys Control\+Alt\+H in Global: Place\.Subcircuit\.Half Adder$', r'^Modified: no$',
			 r'^Chooser: Half Adder Shortcut: Shift\+H \(Circuit\), Ctrl\+Alt\+H$', r'^Chooser: Shift\+J is free\.$',
			 r'^Chooser: Half Adder Shortcut: Shift\+H \(Circuit\), Ctrl\+Alt\+H, Shift\+J \(Circuit\)$', r'^Chooser: closed$', r'^Modified: yes$',
			 r'^Keys Shift\+J in Circuit: Place\.Subcircuit\.Half Adder$', r'^Command Edit\.Undo: run$', r'^Keys Shift\+J in Circuit: none$',
			 r'^Command Edit\.Redo: run$', r'^Command: Place\.Subcircuit\.Main$', r'^Command: Place\.Subcircuit\.Half Adder \[Control\+Alt\+H\] bound$'],
			[r'Unknown check step', r'NOT BOUND', r'not run'], asEnv)
		if bPassed and not ('placeKeys="Circuit:Shift+H;Global:Control+Alt+H;Circuit:Shift+J"' in open(szSaved).read()):
			print('FAIL subcircuit-keys\n    the saved project hasn\'t the keys')
			bPassed = False
		if bPassed and ('Half Adder' in open(szKeymap).read()):
			print('FAIL subcircuit-keys\n    the keymap kept a subcircuit\'s keys')
			bPassed = False
		return bPassed
	finally:
		shutil.rmtree(szDirectory, ignore_errors=True)


def main():
	szExecutable = os.path.join(ROOT, 'build', 'LogiCimRelease', 'LogiCim')
	aszNames = []
	for szArgument in sys.argv[1:]:
		if os.path.isfile(szArgument):
			szExecutable = os.path.abspath(szArgument)
		else:
			aszNames.append(szArgument)
	iFailed = 0
	for szName, szSteps, aszExpected, aszForbidden in CHECKS:
		if aszNames and (szName not in aszNames):
			continue
		if not Check(szExecutable, szName, szSteps, aszExpected, aszForbidden):
			iFailed += 1
	if (not aszNames) or ('remembered-files' in aszNames):
		if not CheckRememberedFiles(szExecutable):
			iFailed += 1
	if (not aszNames) or ('keymap' in aszNames):
		if not CheckKeymap(szExecutable):
			iFailed += 1
	if (not aszNames) or ('subcircuit-keys' in aszNames):
		if not CheckSubcircuitKeys(szExecutable):
			iFailed += 1
	if (not aszNames) or ('dock-layout' in aszNames):
		if not CheckDockLayout(szExecutable):
			iFailed += 1
	if (not aszNames) or ('project-changes' in aszNames):
		if not CheckProjectChanges(szExecutable):
			iFailed += 1
	if (not aszNames) or ('new-project' in aszNames):
		if not CheckNewProject(szExecutable):
			iFailed += 1
	if (not aszNames) or ('recent-projects' in aszNames):
		if not CheckRecentProjects(szExecutable):
			iFailed += 1
	print('%d failed' % iFailed)
	return 1 if iFailed else 0


if __name__ == '__main__':
	sys.exit(main())
