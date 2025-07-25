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

#include <wx/wx.h>
#include <wx/choice.h>
#include <wx/button.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/dirdlg.h>
#include <wx/splitter.h>
#include <wx/scrolwin.h>
#include <wx/wrapsizer.h>

#include "fdicontheme.h"

#include "dvcard.h"


class IconCardRenderer : public wxDataViewCardRenderer {
public:
    IconCardRenderer() = default;

    wxSize GetCardSize(const wxDataViewListModel& model, const wxDataViewItem& item) const override {
        return wxSize(150, 150); // Taille fixe pour les cartes
    }

    void DrawCard(const wxDataViewListModel& model, const wxDataViewItem& item, wxDC& dc, const wxPoint& pos, const wxSize& size) const override {
        wxVariant iconNameVar;
        wxVariant bitmapVar;
        model.GetValue(iconNameVar, item, 0);
        model.GetValue(bitmapVar, item, 1);

        wxString name = iconNameVar.GetString();
        wxBitmap bitmap;
        bitmap << bitmapVar;

        dc.SetTextForeground(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
        dc.SetBrush(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_ACTIVECAPTION), wxBRUSHSTYLE_CROSSDIAG_HATCH));
        dc.SetPen(*wxTRANSPARENT_PEN);

        wxSize textSz = dc.GetTextExtent(name);
        dc.DrawText(name, pos.x + (size.x - textSz.x) / 2, pos.y + size.y - textSz.y);;

        if (bitmap.IsOk()) {
            wxPoint bitmapPos(
                pos.x + (size.x - bitmap.GetWidth()) / 2,
                pos.y + (size.y - textSz.y - 8 - bitmap.GetHeight()) / 2
            );
            dc.DrawRectangle(bitmapPos, bitmap.GetSize());
            dc.DrawBitmap(bitmap, bitmapPos);
        }
    }

    size_t GetFieldCount() const override {
        return 2; // Nom et Bitmap
    }
};

class IconStore : public wxDataViewIndexListModel {
public:
    virtual void GetValueByRow(wxVariant &value, unsigned int row, unsigned int col ) const override {
        if(icons.size()>0 && row<icons.size()) {
            const IconData& iconData = icons[row];
            if(col == 0) {
                value = iconData.name;
            } else if(col == 1) {
                value << iconData.bitmap;
            }
        } else {
            value = wxVariant();
        }
    }

    virtual bool SetValueByRow( const wxVariant &value,
                                unsigned int row, unsigned int col ) override {
        return false;
    }

    void Clear() {
        icons.clear();
        Cleared();
    }

    void AddIcon(const wxString& name, const wxBitmap& bitmap) {
        icons.push_back({name, bitmap});
    }

    void NotifyAllChanged() {
        Reset(icons.size());
    }

protected:
    struct IconData {
        wxString name;
        wxBitmap bitmap;
    };
    wxVector<IconData> icons;
};


class IconThemeViewer : public wxFrame {
public:
    IconThemeViewer() : wxFrame(nullptr, wxID_ANY, "Visualiseur de thèmes d'icônes",
                                wxDefaultPosition, wxSize(1000, 700)) {

        dirManager.AddPath("/usr/share/pixmaps/");
        dirManager.AddPath("/usr/share/icons/");
        dirManager.AddPath("~/.local/share/icons/");
        dirManager.AddPath("~/.icons/");

        iconLocator = new IconLocator(dirManager);


        CreateControls();
        SetupLayout();
        BindEvents();

        Center();

        RefreshThemes();
    }

private:
    wxSplitterWindow* splitter;
    wxPanel* leftPanel;
    wxDataViewCardCtrl* cardCtrl;

    wxStaticText* dirLabel;
    wxStaticText* themeLabel;
    wxStaticText* sizeLabel;

    wxListBox* dirList;
    wxButton* addDirBtn;
    wxButton* removeDirBtn;
    wxChoice* themeChoice;
    wxChoice* sizeChoice;

    // Données
    ThemeDirectoryManager dirManager;
    IconLocator* iconLocator;

    IconStore*  store;
    wxDataViewCardRenderer* cardRenderer;


    void CreateControls() {
        splitter = new wxSplitterWindow(this, wxID_ANY);

        // Left panel - Configuration
        leftPanel = new wxPanel(splitter, wxID_ANY);

        // Directory list
        dirLabel = new wxStaticText(leftPanel, wxID_ANY, "Directories :");
        dirList = new wxListBox(leftPanel, wxID_ANY);
        dirList->Append("/usr/share/pixmaps/");
        dirList->Append("/usr/share/icons/");
        dirList->Append("~/.local/share/icons/");
        dirList->Append("~/.icons/");
        addDirBtn = new wxButton(leftPanel, wxID_ANY, "Add");
        removeDirBtn = new wxButton(leftPanel, wxID_ANY, "Remove");

        // Theme choice
        themeLabel = new wxStaticText(leftPanel, wxID_ANY, "Theme :");
        themeChoice = new wxChoice(leftPanel, wxID_ANY);

        // Size choice
        sizeLabel = new wxStaticText(leftPanel, wxID_ANY, "Size :");
        sizeChoice = new wxChoice(leftPanel, wxID_ANY);
        sizeChoice->Append("16 px");
        sizeChoice->Append("24 px");
        sizeChoice->Append("32 px");
        sizeChoice->Append("48 px");
        sizeChoice->Append("64 px");
        sizeChoice->Append("96 px");
        sizeChoice->Append("128 px");
        sizeChoice->SetSelection(2); // 32 px par défaut

        // Card control for displaying icons
        store = new IconStore();
        store->IncRef();

        cardRenderer = new IconCardRenderer();
        cardRenderer->IncRef();

        cardCtrl = new wxDataViewCardCtrl(splitter, wxID_ANY);
        cardCtrl->AssociateCardRenderer(cardRenderer);
        cardCtrl->AssociateModel(store);
    }

    void SetupLayout() {
        // Left panel layout
        auto leftSizer = new wxBoxSizer(wxVERTICAL);

        leftSizer->Add(dirLabel, 0, wxALL, 5);
        leftSizer->Add(dirList, 1, wxEXPAND | wxALL, 5);

        auto btnSizer = new wxBoxSizer(wxHORIZONTAL);
        btnSizer->Add(addDirBtn, 0, wxALL, 2);
        btnSizer->Add(removeDirBtn, 0, wxALL, 2);
        leftSizer->Add(btnSizer, 0, wxALL, 5);

        leftSizer->Add(themeLabel, 0, wxALL, 5);
        leftSizer->Add(themeChoice, 0, wxEXPAND | wxALL, 5);

        leftSizer->Add(sizeLabel, 0, wxALL, 5);
        leftSizer->Add(sizeChoice, 0, wxEXPAND | wxALL, 5);

        leftPanel->SetSizer(leftSizer);

        // Splitter
        splitter->SplitVertically(leftPanel, cardCtrl, 250);
        splitter->SetMinimumPaneSize(200);

        // Main layout
        auto mainSizer = new wxBoxSizer(wxVERTICAL);
        mainSizer->Add(splitter, 1, wxEXPAND);
        SetSizer(mainSizer);
    }

    void BindEvents() {
        addDirBtn->Bind(wxEVT_BUTTON, &IconThemeViewer::OnAddDirectory, this);
        removeDirBtn->Bind(wxEVT_BUTTON, &IconThemeViewer::OnRemoveDirectory, this);
        themeChoice->Bind(wxEVT_CHOICE, &IconThemeViewer::OnThemeChanged, this);
        sizeChoice->Bind(wxEVT_CHOICE, &IconThemeViewer::OnSizeChanged, this);
    }

    void OnAddDirectory(wxCommandEvent&) {
        wxDirDialog dialog(this, "Choisir un répertoire d'icônes");
        if (dialog.ShowModal() == wxID_OK) {
            wxString path = dialog.GetPath();
            dirManager.AddPath(path);
            dirList->Append(path);
            RefreshThemes();
        }
    }

    void OnRemoveDirectory(wxCommandEvent&) {
        int sel = dirList->GetSelection();
        if (sel != wxNOT_FOUND) {
            dirList->Delete(sel);
            // Remarque : il faudrait aussi supprimer du dirManager
            RefreshThemes();
        }
    }

    void OnThemeChanged(wxCommandEvent&) {
        DisplayIcons();
    }

    void OnSizeChanged(wxCommandEvent&) {
        DisplayIcons();
    }

    void RefreshThemes() {
        wxString theme = themeChoice->GetStringSelection();

        iconLocator->LoadThemes();

        themeChoice->Clear();
        auto themes = iconLocator->GetThemeNames();
        for (const auto& theme : themes) {
            themeChoice->Append(theme);
        }

        if (!themes.empty()) {
            if(!theme.IsEmpty()) {
                themeChoice->SetSelection(themeChoice->FindString(theme));
            } else {
                themeChoice->SetSelection(0);
            }
            DisplayIcons();
        }
    }

    void DisplayIcons() {
        store->Clear();

        if (themeChoice->GetSelection() == wxNOT_FOUND) {
            return;
        }

        wxString themeName = themeChoice->GetStringSelection();
        int sizeIndex = sizeChoice->GetSelection();
        if (sizeIndex == wxNOT_FOUND) return;

        int sizes[] = {16, 24, 32, 48, 64, 96, 128};
        int iconSize = sizes[sizeIndex];


        // Créer une grille d'icônes
        auto gridSizer = new wxWrapSizer(wxHORIZONTAL);

        // Obtenir tous les noms d'icônes du thème
        auto iconNames = GetIconNamesFromTheme(themeName);

        for (const auto& iconName : iconNames) {

            auto iconFile = iconLocator->FindIcon(themeName, iconName, iconSize);
            if (iconFile) {
                wxBitmap bitmap(iconFile->GetFullPath(), wxBITMAP_TYPE_ANY);
                if (bitmap.IsOk()) {
                    // Redimensionner si nécessaire
                    if (bitmap.GetWidth() != iconSize || bitmap.GetHeight() != iconSize) {
                        wxImage image = bitmap.ConvertToImage();
                        image = image.Scale(iconSize, iconSize, wxIMAGE_QUALITY_HIGH);
                        bitmap = wxBitmap(image);
                    }

                    store->AddIcon(iconName, bitmap);
                }
            }
        }

        store->NotifyAllChanged();
        cardCtrl->Refresh();
    }

    std::set<wxString> GetIconNamesFromTheme(const wxString& themeName) {
        if (themeName.IsEmpty()) return {};
        return iconLocator->GetIconNames(themeName);
    }
};

class MainApp : public wxApp {
public:
    bool OnInit() override {
        if (!wxApp::OnInit())
            return false;

        auto frame = new IconThemeViewer();
        frame->Show();
        return true;
    }
};

wxIMPLEMENT_APP(MainApp);
