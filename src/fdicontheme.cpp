/*
 * wxFreeDesktopIconTheme - A wxWidgets FreeDesktop Icon Theme support.
 * Copyright (C) 2025 Emilien KIA
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/
#include "fdicontheme.h"

#include <wx/dir.h>
#include <wx/log.h>
#include <wx/filename.h>
#include <wx/wfstream.h>
#include <wx/tokenzr.h>
#include <wx/filefn.h>

#include <filesystem>

//
// IconTheme
//


IconTheme::IconTheme(const wxString& themePath) : path(themePath) {}

bool IconTheme::Preload() {
    wxFileName indexFile(path, "index.theme");
    if (!indexFile.FileExists()) return false;

    wxFileInputStream input(indexFile.GetFullPath());
    if (!input.IsOk()) return false;

    wxFileConfig config(input);

    wxString dirList;
    if (!config.Read("Icon Theme/Directories", &dirList)) return false;
    name = config.Read("Icon Theme/Name", path.AfterLast(wxFileName::GetPathSeparator()));

    wxString inheritsStr;
    if (config.Read("Icon Theme/Inherits", &inheritsStr)) {
        wxStringTokenizer tk(inheritsStr, ",");
        while (tk.HasMoreTokens()) {
            inherits.push_back(tk.GetNextToken().Trim().Trim(false));
        }
    }

    wxStringTokenizer dirs(dirList, ",");
    while (dirs.HasMoreTokens()) {
        wxString dirName = dirs.GetNextToken().Trim().Trim(false);
        wxString section = dirName;
        if (!config.HasGroup(section)) continue;

        config.SetPath("/" + section);
        IconDirectory dir;
        dir.path = wxFileName(path + "/" + section , "").GetFullPath();
        config.Read("Size", &dir.size);
        config.Read("MinSize", &dir.minSize);
        config.Read("MaxSize", &dir.maxSize);
        config.Read("Type", &dir.type);
        directories.push_back(dir);
        config.SetPath("/");
    }

    return true;
}

void IconTheme::BuildCache(bool force) const {
    if(iconCache.size()==0 || force) {
        iconCache.clear();
        for (const auto& dir : directories) {
            wxDir directory(dir.path);
            if (!directory.IsOpened()) continue;

            wxString file;
            bool cont = directory.GetFirst(&file, "*.png", wxDIR_FILES);
            while (cont) {
                wxFileName full(dir.path, file);
                wxString iconName = full.GetName();  // sans extension
                iconCache[iconName][dir.size] = full;

                cont = directory.GetNext(&file);
            }
        }
    }
}

std::optional<wxFileName> IconTheme::FindIcon(const wxString& iconName, int size) {
    BuildCache();
    if (iconCache.count(iconName)) {
        const auto& sizeMap = iconCache[iconName];
        if (sizeMap.count(size)) return sizeMap.at(size);
        // Approximate match
        int closest = -1;
        for (const auto& [sz, _] : sizeMap) {
            if (closest == -1 || std::abs(sz - size) < std::abs(closest - size))
                closest = sz;
        }
        if (closest != -1) return sizeMap.at(closest);
    }
    return std::nullopt;
}

std::map<int, wxFileName> IconTheme::FindAllIcons(const wxString& iconName) const {
    BuildCache();
    std::map<int, wxFileName> results;
    auto it = iconCache.find(iconName);
    if (it != iconCache.end()) {
        for (const auto& [size, file] : it->second) {
            results[size] = file;
        }
    }
    return results;
}

std::set<wxString> IconTheme::GetIconNames() const {
    BuildCache();
    std::set<wxString> names;
    for (const auto& [name, _] : iconCache) {
        names.insert(name);
    }
    return names;
}

//
// ThemeDirectoryManager
//

void ThemeDirectoryManager::AddPath(const wxString& path) {
    if (wxDirExists(path)) {
        paths.push_back(path);
    }
}

const wxVector<wxString>& ThemeDirectoryManager::GetPaths() const {
    return paths;
}

//
// FreeDesktopIconProvider
//

FreeDesktopIconProvider::FreeDesktopIconProvider()
{
}

FreeDesktopIconProvider::FreeDesktopIconProvider(const wxVector<wxString>& paths)
{
    for(const auto& path : paths) {
        AppendPath(path);
    }
}

void FreeDesktopIconProvider::Clear()
{
    directories.clear();
    themes.clear();
}

void FreeDesktopIconProvider::AppendPath(const wxString& path)
{
    // TODO Ensure the path iis not already in the list
    wxFileName dirPath(path);
    if (wxDirExists(dirPath.GetFullPath())) {
        directories.push_back(std::move(LoadThemesFromDirectory(dirPath)));
    }/* else {
        wxLogWarning("Directory does not exist: %s", dirPath.GetFullPath());
    }*/
}

void FreeDesktopIconProvider::PrependPath(const wxString& path)
{
    // TODO Ensure the path iis not already in the list
    wxFileName dirPath(path);
    if (wxDirExists(dirPath.GetFullPath())) {
        directories.insert(directories.begin(), std::move(LoadThemesFromDirectory(dirPath)));
    }/* else {
        wxLogWarning("Directory does not exist: %s", dirPath.GetFullPath());
    }*/
}

void FreeDesktopIconProvider::RemovePath(const wxString& path)
{
    wxString fullPath = wxFileName(path).GetFullPath();
    auto it = std::find_if(directories.begin(), directories.end(), [&](const ThemeDirectory& dir)-> bool { return dir.path == fullPath; });
    if(it!= directories.end()) {
        for(const auto& theme : it->themes) {
            themes.erase(theme.second); // Remove theme from the main map
        }
        directories.erase(it);
    }/* else {
        wxLogWarning("Directory not found: %s", fullPath);
    }*/
}

ThemeDirectory FreeDesktopIconProvider::LoadThemesFromDirectory(const wxFileName& dirPath)
{
    ThemeDirectory themeDir;
    themeDir.path = dirPath.GetFullPath();

    wxDir dir(themeDir.path);
    wxString sub;
    bool cont = dir.GetFirst(&sub, wxEmptyString, wxDIR_DIRS);
    while (cont) {
        wxFileName themePath(themeDir.path, sub);
        IconTheme theme(themePath.GetFullPath());
        if (theme.Preload()) {
            themeDir.themes.insert({themePath.GetFullPath(), theme.GetName()});
            themes.insert({theme.GetName(), std::move(theme)});
        }
        cont = dir.GetNext(&sub);
    }
    return std::move(themeDir);
}

wxVector<wxString> FreeDesktopIconProvider::GetThemeNames() const {
    wxVector<wxString> names;
    for (const auto& [name, _] : themes) {
        names.push_back(name);
    }
    return names;
}

std::set<wxString> FreeDesktopIconProvider::GetIconNames(const wxString& themeName) const
{
    std::set<wxString> res;
    auto it = themes.find(themeName);
    if (it != themes.end())
    {
        auto found = it->second.GetIconNames();
        res.insert(found.begin(), found.end());
        for (const auto& parent : it->second.GetInherits()) {
            auto fallback = GetIconNames(parent);
            res.insert(fallback.begin(), fallback.end());
        }
    }
    return res;
}

std::set<wxString> FreeDesktopIconProvider::GetIconNames() const
{
    return GetIconNames(currentTheme);
}



std::optional<wxFileName> FreeDesktopIconProvider::FindIcon(const wxString& iconName, int size) {
    return FindIcon(currentTheme, iconName, size);
}

std::optional<wxFileName> FreeDesktopIconProvider::FindIcon(const wxString& theme, const wxString& iconName, int size) {
    auto it = themes.find(theme);
    if (it == themes.end()) return std::nullopt;

    auto found = it->second.FindIcon(iconName, size);
    if (found) return found;

    for (const auto& parent : it->second.GetInherits()) {
        auto fallback = FindIcon(parent, iconName, size);
        if (fallback) return fallback;
    }

    return std::nullopt;
}

std::optional<wxBitmapBundle> FreeDesktopIconProvider::LoadIconBundle(const wxString& iconName) {
    std::map<int, wxFileName> foundIcons;

    std::set<wxString> visited;

    // Recurse through current + inherited themes
    std::function<void(const wxString&)> collectIcons = [&](const wxString& themeName) {
        if (visited.count(themeName)) return;
        visited.insert(themeName);

        auto it = themes.find(themeName);
        if (it == themes.end()) return;

        auto subIcons = it->second.FindAllIcons(iconName);
        for (const auto& [size, file] : subIcons) {
            foundIcons[size] = file;
        }

        for (const auto& parent : it->second.GetInherits()) {
            collectIcons(parent);
        }
    };

    collectIcons(currentTheme);

    if (foundIcons.empty()) return std::nullopt;

    wxVector<wxBitmap> bitmaps;
    for (const auto& [size, file] : foundIcons) {
        // TODO Preload through wxImage to handle SVGs and PNGs
        wxBitmap bmp(file.GetFullPath(), wxBITMAP_TYPE_ANY);
        if (bmp.IsOk())
            bitmaps.push_back(bmp);
    }

    if (bitmaps.empty()) return std::nullopt;

    return wxBitmapBundle::FromBitmaps(bitmaps);
}
