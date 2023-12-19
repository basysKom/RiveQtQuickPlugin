# **Build Configuration**

---

## **Windows**

### **Building**

You can create your own batch files like `build.bat` file in the root of the project with these codes:

```bat
@REM SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
@REM SPDX-FileCopyrightText: 2023 Jonas Kalinka <jonas.kalinka@basyskom.com>
@REM SPDX-FileCopyrightText: 2023 basysKom GmbH
@REM SPDX-FileCopyrightText: 2023 Reza Aarabi <madoodia@gmail.com>

@REM SPDX-License-Identifier: LGPL-3.0-or-later
@REM --------------------------------------------------------------------------

@REM Getting the Root Directory When this file will be run
set ROOT=%~dp0


@REM VCVARS_LOCATION: C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build
@REM If you have installed other versions of Visual Studio Build Tools like 2019 you can set its path to this Environment Variable
echo  %VCVARS_LOCATION%

@REM Setting vcvars_ver flag is important
@REM Visual Studio 2019 Compiler
@REM call "%VCVARS_LOCATION%/vcvarsall.bat" x64 -vcvars_ver=14.29.30133

@REM Visual Studio 2022 Compiler
call "%VCVARS_LOCATION%/vcvarsall.bat" x64 -vcvars_ver=14.36.32532



cd %ROOT%

if not exist "build" (
    mkdir "build"
)


@REM REM Qt5_DIR
@REM set Qt5_DIR=%SDKS_LOCATION%/Qt5/lib/cmake
@REM set PATH=%PATH%;%SDKS_LOCATION%/Qt5/bin

REM Qt6_DIR
set Qt6_DIR=%SDKS_LOCATION%/Qt6/lib/cmake
set PATH=%PATH%;%SDKS_LOCATION%/Qt6/bin

cd "%ROOT%/build"

@REM cmake -G "Visual Studio 16 2019" -A x64 -DCMAKE_PREFIX_PATH=%Qt6_DIR%  -DCMAKE_CXX_FLAGS="/bigobj" "%ROOT%"
cmake -G "NMake Makefiles" -DCMAKE_PREFIX_PATH=%Qt6_DIR%  -DCMAKE_CXX_FLAGS="/bigobj" "%ROOT%"
@REM cmake --build . --config Debug
cmake --build . --config Release

```

### **Running**

You can create your own batch files like `run.bat` file in the root of the project with these codes:

```bat
@REM SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
@REM SPDX-FileCopyrightText: 2023 Jonas Kalinka <jonas.kalinka@basyskom.com>
@REM SPDX-FileCopyrightText: 2023 basysKom GmbH
@REM SPDX-FileCopyrightText: 2023 Reza Aarabi <madoodia@gmail.com>

@REM SPDX-License-Identifier: LGPL-3.0-or-later
@REM --------------------------------------------------------------------------

@REM Getting the Root Directory When this file will be run
set ROOT=%~dp0

@REM set PATH=%PATH%;%SDKS_LOCATION%/Qt5/bin
set PATH=%PATH%;%SDKS_LOCATION%/Qt6/bin

set QML2_IMPORT_PATH=%ROOT%\build\binary

CALL %ROOT%\build\binary\examples\RiveQtQuickPlugin\RiveQtQuickSimpleViewer.exe

```

### **Deployment**

If you want to make a package of the tool and ship it, you can create your own `deploy.bat` file in the root of the project with these codes:

```bat
@REM SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
@REM SPDX-FileCopyrightText: 2023 Jonas Kalinka <jonas.kalinka@basyskom.com>
@REM SPDX-FileCopyrightText: 2023 basysKom GmbH
@REM SPDX-FileCopyrightText: 2023 Reza Aarabi <madoodia@gmail.com>

@REM SPDX-License-Identifier: LGPL-3.0-or-later
@REM --------------------------------------------------------------------------

@REM Getting the Root Directory When this file will be run
set ROOT=%~dp0

cd %ROOT%
if not exist "deploy" (
    mkdir "deploy"
)


copy %ROOT%\build\binary\examples\RiveQtQuickPlugin\RiveQtQuickSimpleViewer.exe "%ROOT%\deploy"

CALL %SDKS_LOCATION%\Qt6\bin\windeployqt.exe %ROOT%\deploy\RiveQtQuickSimpleViewer.exe --no-translations

```
And copy `RiveQtQuickPlugin.dll` to deploy directory from `build\binary\RiveQtQuickPlugin\RiveQtQuickPlugin.dll`

Please read the [README.md](https://github.com/madoodia/RiveQtQuickPlugin/blob/main/README.md) file.
