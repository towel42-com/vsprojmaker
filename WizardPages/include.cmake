# The MIT License (MIT)
#
# Copyright (c) 2020-2021 Scott Aron Bloom
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sub-license, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

set(_PROJECT_NAME WizardPages)
set(USE_QT TRUE)
set(FOLDER_NAME Libs)

set(qtproject_SRCS
    ClientInfoPage.cpp
    BuildTargetsPage.cpp
    DebugTargetsPage.cpp
    IncludesPage.cpp
    PreProcDefinesPage.cpp
    QtPage.cpp
    SystemInfoPage.cpp
)

set(qtproject_H
    ClientInfoPage.h
    BuildTargetsPage.h
    DebugTargetsPage.h
    IncludesPage.h
    PreProcDefinesPage.h
    QtPage.h
    SystemInfoPage.h
)

set(project_H
)

set(qtproject_UIS
    ClientInfoPage.ui
    BuildTargetsPage.ui
    DebugTargetsPage.ui
    IncludesPage.ui
    PreProcDefinesPage.ui
    QtPage.ui
    SystemInfoPage.ui
)

set(qtproject_QRC
)
