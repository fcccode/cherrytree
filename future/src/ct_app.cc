/*
 * ct_app.cc
 * 
 * Copyright 2017-2018 Giuseppe Penone <giuspen@gmail.com>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#include <glibmm/i18n.h>
#include <glib/gstdio.h>
#include <iostream>
#include "ct_app.h"


CtConfig* CTApplication::P_ctCfg{nullptr};
Glib::RefPtr<Gtk::IconTheme> CTApplication::R_icontheme;
CTTmp* CTApplication::P_ctTmp{nullptr};
Glib::RefPtr<Gtk::TextTagTable> CTApplication::R_textTagTable;
Glib::RefPtr<Gsv::LanguageManager> CTApplication::R_languageManager;
Glib::RefPtr<Gsv::StyleSchemeManager> CTApplication::R_styleSchemeManager;


CTApplication::CTApplication() : Gtk::Application("com.giuspen.cherrytree", Gio::APPLICATION_HANDLES_OPEN)
{
    Gsv::init();

    if (nullptr == P_ctCfg)
    {
        P_ctCfg = new CtConfig();
        //std::cout << P_ctCfg->specialChars.size() << "\t" << P_ctCfg->specialChars << std::endl;
    }
    if (!R_icontheme)
    {
        _iconthemeInit();
    }
    if (nullptr == P_ctTmp)
    {
        P_ctTmp = new CTTmp();
        //std::cout << P_ctTmp->get_root_dirpath() << std::endl;
    }
    if (!R_textTagTable)
    {
        R_textTagTable = Gtk::TextTagTable::create();
    }
    if (!R_languageManager)
    {
        R_languageManager = Gsv::LanguageManager::create();
    }
    if (!R_styleSchemeManager)
    {
        R_styleSchemeManager = Gsv::StyleSchemeManager::create();
    }
}

CTApplication::~CTApplication()
{
    //std::cout << "~CTApplication()" << std::endl;
    delete P_ctCfg;
    P_ctCfg = nullptr;

    delete P_ctTmp;
    P_ctTmp = nullptr;
}

Glib::RefPtr<CTApplication> CTApplication::create()
{
    return Glib::RefPtr<CTApplication>(new CTApplication());
}

void CTApplication::_printHelpMessage()
{
    std::cout << "Usage: " << GETTEXT_PACKAGE << " filepath[.ctd|.ctb]" << std::endl;
}

void CTApplication::_printGresourceIcons()
{
    for (std::string &str_icon : Gio::Resource::enumerate_children_global("/icons/", Gio::ResourceLookupFlags::RESOURCE_LOOKUP_FLAGS_NONE))
    {
        std::cout << str_icon << std::endl;
    }
}

void CTApplication::_iconthemeInit()
{
    R_icontheme = Gtk::IconTheme::get_default();
    R_icontheme->add_resource_path("/icons/");
    //_printGresourceIcons();
}

MainWindow* CTApplication::create_appwindow()
{
    auto p_main_win = new MainWindow();

    add_window(*p_main_win);

    return p_main_win;
}

void CTApplication::on_activate()
{
    // app run without arguments
    auto p_appwindow = create_appwindow();
    p_appwindow->present();
}

void CTApplication::on_open(const Gio::Application::type_vec_files& files, const Glib::ustring& /* hint */)
{
    // app run with arguments
    MainWindow* p_appwindow = nullptr;
    auto windows_list = get_windows();
    if (windows_list.size() > 0)
    {
        p_appwindow = dynamic_cast<MainWindow*>(windows_list[0]);
    }

    if (!p_appwindow)
    {
        p_appwindow = create_appwindow();
    }

    for (const Glib::RefPtr<Gio::File>& r_file : files)
    {
        if(r_file->query_exists())
        {
            if(!p_appwindow->readNodesFromGioFile(r_file))
            {
                _printHelpMessage();
            }
        }
        else
        {
            std::cout << "!! Missing file " << r_file->get_path() << std::endl;
        }
    }

    p_appwindow->present();
}


CTTmp::CTTmp()
{
}

CTTmp::~CTTmp()
{
    //std::cout << "~CTTmp()" << std::endl;
    for (const auto& currPair : _mapHiddenFiles)
    {
        if (g_file_test(currPair.second, G_FILE_TEST_IS_REGULAR) && (0 != g_remove(currPair.second)))
        {
            std::cerr << "!! g_remove" << std::endl;
        }
        g_free(currPair.second);
    }
    for (const auto& currPair : _mapHiddenDirs)
    {
        if (g_file_test(currPair.second, G_FILE_TEST_IS_DIR) && (0 != g_rmdir(currPair.second)))
        {
            std::cerr << "!! g_rmdir" << std::endl;
        }
        g_free(currPair.second);
    }
}

const gchar* CTTmp::getHiddenDirPath(const std::string& visiblePath)
{
    if (!_mapHiddenDirs.count(visiblePath))
    {
        _mapHiddenDirs[visiblePath] = g_dir_make_tmp(NULL, NULL);
    }
    return _mapHiddenDirs.at(visiblePath);
}

const gchar* CTTmp::getHiddenFilePath(const std::string& visiblePath)
{
    if (!_mapHiddenFiles.count(visiblePath))
    {
        const gchar* tempDir{getHiddenDirPath(visiblePath)};
        std::string basename{Glib::path_get_basename(visiblePath)};
        if (Glib::str_has_suffix(basename, ".ctx"))
        {
            basename.replace(basename.end()-1, basename.end(), "b");
        }
        else if (Glib::str_has_suffix(basename, ".ctz"))
        {
            basename.replace(basename.end()-1, basename.end(), "d");
        }
        else
        {
            std::cerr << "!! unexpected basename " << basename << std::endl;
        }
        _mapHiddenFiles[visiblePath] = g_build_filename(tempDir, basename.c_str(), NULL);
    }
    return _mapHiddenFiles.at(visiblePath);
}
