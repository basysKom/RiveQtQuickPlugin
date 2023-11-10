#[[
  SPDX-FileCopyrightText: 2023 Jeremias Bosch <jeremias.bosch@basyskom.com>
  SPDX-FileCopyrightText: 2023 basysKom GmbH

  SPDX-License-Identifier: LGPL-3.0-or-later
]]

if (${QT_VERSION_MAJOR} EQUAL 6)
    add_subdirectory(hellorive)
endif()

add_subdirectory(SimpleViewer)
