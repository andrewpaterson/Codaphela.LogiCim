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
NUM_ACTIONS = 40	#ELogicimAction's actions, each a command, with the three commands that only have keys.

#Each check: its name, its LOGICIM_CHECK steps, the lines (regular expressions) its output must have, in order, and
#lines it mustn't have.
CHECKS = [
	('commands', 'commands',
		[r'^Commands: %d$' % (NUM_ACTIONS + 3),
		 r'^Command: File\.LoadProject$',
		 r'^Command: Edit\.FlipHorizontally \[Control\+Shift\+F, H\] bound$',
		 r'^Command: Edit\.FlipVertically \[Control\+Shift\+F, V\] bound$',
		 r'^Command: File\.Exit$',
		 r'^Command: Edit\.StepUndo \[Control\+Alt\+Z\] bound$',
		 r'^Command: Edit\.StepRedo \[Control\+Alt\+Shift\+Z\] bound$',
		 r'^Command: Circuit\.LeaveSubcircuit \[Alt\+Backspace\] bound$'],
		[r'NOT BOUND']),
	('menus-and-buttons', 'menus,buttons',
		[r'^Menu File: Load Project \[File\.LoadProject\] Save Project \[File\.SaveProject\] Import KiCad Schematic\.\.\. \[File\.ImportKicadSchematic\] Exit \[File\.Exit\]$',
		 r'^Menu Edit: Undo \[Edit\.Undo\] Redo \[Edit\.Redo\] ',
		 r'^Menu View: ',
		 r'^Menu Simulation: ',
		 r'^Menu Place: ',
		 r'^Buttons: [1-9][0-9]* with a command, 0 without$'],
		[r'\[none\]']),
	('run-commands', 'zoom,command:View.ZoomIn,zoom,command:View.ZoomOut,command:View.ZoomOut,zoom,command:View.ResetZoom,zoom',
		[r'^Zoom: 1\.00$', r'^Command View\.ZoomIn: run$', r'^Zoom: 2\.00$', r'^Command View\.ZoomOut: run$',
		 r'^Command View\.ZoomOut: run$', r'^Zoom: (0\.\d\d|1\.00)$', r'^Command View\.ResetZoom: run$', r'^Zoom: 1\.00$'],
		[r'Unknown check step', r'not run']),
]


def Run(szExecutable, szSteps, asExtraEnv=None):
	asEnv = dict(os.environ)
	asEnv.pop('LOGICIM_SETTINGS', None)
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
	print('%d failed' % iFailed)
	return 1 if iFailed else 0


if __name__ == '__main__':
	sys.exit(main())
