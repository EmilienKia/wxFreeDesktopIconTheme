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

#ifndef WXFDICONTHEME_DVCARD_H
#define WXFDICONTHEME_DVCARD_H

#include <wx/wx.h>

#include <wx/dataview.h>
#include <wx/control.h>
#include <map>

class wxDataViewCardRenderer : public wxRefCounter
{
public:
    wxDataViewCardRenderer() = default;
    virtual ~wxDataViewCardRenderer() = default;

    virtual wxSize GetCardSize(const wxDataViewListModel& model, const wxDataViewItem& item, const wxDC& dc) const =0;
    virtual void DrawCard(const wxDataViewListModel& model, const wxDataViewItem& item, wxDC& dc, const wxPoint& pos, const wxSize& size) const =0;

    virtual size_t GetFieldCount() const =0;
};


class wxDataViewCardCtrl : public /*wxDataViewCtrl*/wxControl, protected wxDataViewModelNotifier {
    wxDECLARE_DYNAMIC_CLASS(wxDataViewCardCtrl);
    wxDECLARE_EVENT_TABLE();
public:
    wxDataViewCardCtrl();
    wxDataViewCardCtrl(wxWindow* parent, wxWindowID id = wxID_ANY,
                       const wxPoint& pos = wxDefaultPosition,
                       const wxSize& size = wxDefaultSize,
                       long style = 0, const wxString& name = "wxDataViewCardCtrl");
    virtual ~wxDataViewCardCtrl();

    bool Create(wxWindow* parent, wxWindowID id = wxID_ANY,
                 const wxPoint& pos = wxDefaultPosition,
                 const wxSize& size = wxDefaultSize,
                 long style = 0, const wxString& name = "wxDataViewCardCtrl");

    void AssociateCardRenderer(wxDataViewCardRenderer* renderer);

    void AssociateModel(wxDataViewListModel* model);

    wxDataViewListModel* GetModel() const { return _model; }

protected:
    wxDataViewListModel* _model = nullptr;
    wxDataViewCardRenderer* _renderer = nullptr;

    std::map<void*, wxSize> _cardSizes;
    wxSize _maxSize;
    wxSize _marginSize {8, 8};

    void ComputeCardSize(const wxDataViewItem &item);
    void ComputeCardSizes(const wxDataViewItemArray &items);
    void RecalculateMaxSize();
    void UpdateScrollbars();

    void CommonInit();

    void OnPaint(wxPaintEvent& event);
    void OnSize(wxSizeEvent& event);
    void OnScroll(wxScrollWinEvent& event);

    bool ItemAdded( const wxDataViewItem &parent, const wxDataViewItem &item ) override;
    bool ItemDeleted( const wxDataViewItem &parent, const wxDataViewItem &item ) override;
    bool ItemChanged( const wxDataViewItem &item ) override;
    bool ItemsAdded( const wxDataViewItem &parent, const wxDataViewItemArray &items ) override;
    bool ItemsDeleted( const wxDataViewItem &parent, const wxDataViewItemArray &items ) override;
    bool ItemsChanged( const wxDataViewItemArray &items ) override;
    bool ValueChanged( const wxDataViewItem &item, unsigned int col ) override;
    bool Cleared() override;
    void Resort() override;
};


#endif //WXFDICONTHEME_DVCARD_H
