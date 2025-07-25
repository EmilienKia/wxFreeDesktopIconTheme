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

#ifndef WXFDICONTHEME_FDICONTHEME_HPP
#define WXFDICONTHEME_FDICONTHEME_HPP

#include <wx/wx.h>

#include <wx/string.h>
#include <wx/filename.h>
#include <wx/fileconf.h>
#include <map>
#include <set>
#include <optional>


struct IconDirectory {
    wxString path;
    int size = 0;
    int minSize = 0;
    int maxSize = 0;
    wxString type;
};

class IconTheme {
public:
    IconTheme(const wxString& themePath);
    IconTheme(const IconTheme&) = default;
    IconTheme(IconTheme&&) = default;
    IconTheme& operator=(const IconTheme&) = default;
    IconTheme& operator=(IconTheme&&) = default;

    bool Load();
    const wxString& GetName() const { return name; }
    const wxVector<IconDirectory>& GetDirectories() const { return directories; }
    const wxVector<wxString>& GetInherits() const { return inherits; }

    std::optional<wxFileName> FindIcon(const wxString& iconName, int size);

    std::map<int, wxFileName> FindAllIcons(const wxString& iconName) const;

    std::set<wxString> GetIconNames() const;

private:
    wxString path;
    wxString name;
    wxVector<wxString> inherits;
    wxVector<IconDirectory> directories;

    std::map<wxString, std::map<int, wxFileName>> iconCache;

    void BuildCache();
};


class ThemeDirectoryManager {
public:
    void AddPath(const wxString& path);
    const wxVector<wxString>& GetPaths() const;

private:
    wxVector<wxString> paths;
};


class IconLocator {
public:
    IconLocator(ThemeDirectoryManager& dirManager);
    void LoadThemes();

    wxVector<wxString> GetThemeNames() const;

    std::set<wxString> GetIconNames(const wxString& themeName) const;
    std::set<wxString> GetIconNames() const;

    std::optional<wxFileName> FindIcon(const wxString& iconName, int size);
    std::optional<wxFileName> FindIcon(const wxString& theme, const wxString& iconName, int size);

    std::optional<wxBitmapBundle> LoadIconBundle(const wxString& iconName);


private:
    ThemeDirectoryManager& dirs;
    std::map<wxString, IconTheme> themes;
    wxString currentTheme = "hicolor";

};



#endif //WXFDICONTHEME_FDICONTHEME_HPP
