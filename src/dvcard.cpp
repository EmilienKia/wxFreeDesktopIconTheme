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

#include "dvcard.h"

#include <wx/dcbuffer.h>

IMPLEMENT_DYNAMIC_CLASS(wxDataViewCardCtrl, wxDataViewCtrl)

BEGIN_EVENT_TABLE(wxDataViewCardCtrl, wxDataViewCtrl)
    EVT_PAINT(wxDataViewCardCtrl::OnPaint)
    EVT_SIZE(wxDataViewCardCtrl::OnSize)
    EVT_SCROLLWIN(wxDataViewCardCtrl::OnScroll)
END_EVENT_TABLE()

wxDataViewCardCtrl::wxDataViewCardCtrl() :
wxControl()
{
}

wxDataViewCardCtrl::wxDataViewCardCtrl(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name):
wxControl(parent, id, pos, size, style|wxVSCROLL, wxDefaultValidator, name)
{
    CommonInit();
}

wxDataViewCardCtrl::~wxDataViewCardCtrl()
{
    if(_renderer != nullptr) {
        _renderer->DecRef();
        _renderer = nullptr;
    }
    if(_model != nullptr) {
        _model->DecRef();
        _model = nullptr;
    }
}

bool wxDataViewCardCtrl::Create(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name)
{
    if(!wxControl::Create(parent, id, pos, size, style|wxVSCROLL, wxDefaultValidator, name)) {
        return false;
    }
    CommonInit();
    return true;
}

void wxDataViewCardCtrl::CommonInit()
{
    wxWindow::SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
}

void wxDataViewCardCtrl::AssociateCardRenderer(wxDataViewCardRenderer* renderer)
{
    if(_renderer != renderer) {
        if(_renderer != nullptr) {
            _renderer->DecRef();
        }
        _renderer = renderer;
        if(_renderer) {
            _renderer->IncRef();
        }
    }
}

void wxDataViewCardCtrl::AssociateModel(wxDataViewListModel* model)
{
    if(_model != model) {
        if(_model != nullptr) {
            _model->RemoveNotifier(this);
            _model->DecRef();
        }
        _model = model;
        if(_model) {
            _model->IncRef();
            _model->AddNotifier(this);
        }
    }
}

void wxDataViewCardCtrl::OnPaint(wxPaintEvent& event)
{
    wxSize clientSize = GetClientSize();

    wxAutoBufferedPaintDC dc(this);
    dc.SetBrush(wxBrush(GetBackgroundColour()));
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRectangle(wxPoint(0, 0), clientSize);

    wxDataViewListModel* model = GetModel();
    if(model)
    {
        wxDataViewItemArray children;
        wxDataViewItem root;
        unsigned int count = model->GetChildren(root, children);

        wxPoint pos{_marginSize.GetWidth(), _marginSize.GetHeight()};;

        int cardPerRow = (clientSize.GetWidth()-_marginSize.GetWidth()) / (_maxSize.GetWidth()+_marginSize.GetWidth());
        if(cardPerRow <= 0) {
            cardPerRow = 1; // At least one card per row
        }
        int firstCard = GetScrollPos(wxVERTICAL) * cardPerRow;

        for(unsigned int i = firstCard; i < count; ++i)
        {
            wxDataViewItem item = children[i];
            dc.SetClippingRegion(pos, _maxSize);
            _renderer->DrawCard(*model, item, dc, pos, _maxSize);
            dc.DestroyClippingRegion();
            pos.x += _maxSize.GetWidth() + _marginSize.GetWidth();

            if(pos.x + _maxSize.GetWidth() + _marginSize.GetWidth() > clientSize.GetWidth()) {
                // If next card would overflow, move to next row
                pos.x = _marginSize.GetWidth();
                pos.y += _maxSize.GetHeight() + _marginSize.GetHeight();
            }

            if(pos.y >= clientSize.GetHeight()) {
                // If next card would overflow vertically, stop drawing
                break;
            }
        }
    }
}

void wxDataViewCardCtrl::OnSize(wxSizeEvent& event)
{
    UpdateScrollbars();
    Refresh();
}

void wxDataViewCardCtrl::OnScroll(wxScrollWinEvent& event)
{
    UpdateScrollbars();
    Refresh();
}

bool wxDataViewCardCtrl::ItemAdded( const wxDataViewItem &parent, const wxDataViewItem &item )
{
    ComputeCardSize(item);
    UpdateScrollbars();
    return true;
}

bool wxDataViewCardCtrl::ItemDeleted( const wxDataViewItem &parent, const wxDataViewItem &item )
{
    _cardSizes.erase(item.GetID());
    RecalculateMaxSize();
    UpdateScrollbars();
    return true;
}

bool wxDataViewCardCtrl::ItemChanged( const wxDataViewItem &item )
{
    ComputeCardSize(item);
    UpdateScrollbars();
    return true;
}

bool wxDataViewCardCtrl::ItemsAdded( const wxDataViewItem &parent, const wxDataViewItemArray &items )
{
    ComputeCardSizes(items);
    UpdateScrollbars();
    return true;
}

bool wxDataViewCardCtrl::ItemsDeleted( const wxDataViewItem &parent, const wxDataViewItemArray &items )
{
    for(const auto& item : items) {
        _cardSizes.erase(item.GetID());
    }
    RecalculateMaxSize();
    UpdateScrollbars();
    return true;
}

bool wxDataViewCardCtrl::ItemsChanged( const wxDataViewItemArray &items )
{
    ComputeCardSizes(items);
    UpdateScrollbars();
    return true;
}

bool wxDataViewCardCtrl::ValueChanged( const wxDataViewItem &item, unsigned int col )
{
    ComputeCardSize(item);
    UpdateScrollbars();
    return true;
}

bool wxDataViewCardCtrl::Cleared()
{
    _cardSizes.clear();
    RecalculateMaxSize();
    UpdateScrollbars();
    return true;
}

void wxDataViewCardCtrl::Resort()
{
    // Do nothing
}

void wxDataViewCardCtrl::ComputeCardSize(const wxDataViewItem &item)
{
    wxClientDC dc(this);
    if(_model && _renderer) {
        wxSize size = _renderer->GetCardSize(*_model, item, dc);
        _cardSizes[item.GetID()] = size;
        if(size.GetWidth() > _maxSize.GetWidth()) {
            _maxSize.SetWidth(size.GetWidth());
        }
        if(size.GetHeight() > _maxSize.GetHeight()) {
            _maxSize.SetHeight(size.GetHeight());
        }
    }
}

void wxDataViewCardCtrl::ComputeCardSizes(const wxDataViewItemArray &items)
{
    wxClientDC dc(this);
    if(_model && _renderer) {
        for(const auto& item : items) {
            wxSize size = _renderer->GetCardSize(*_model, item, dc);
            _cardSizes[item.GetID()] = size;
            if(size.GetWidth() > _maxSize.GetWidth()) {
                _maxSize.SetWidth(size.GetWidth());
            }
            if(size.GetHeight() > _maxSize.GetHeight()) {
                _maxSize.SetHeight(size.GetHeight());
            }
        }
    }
}

void wxDataViewCardCtrl::RecalculateMaxSize()
{
    _maxSize = {0, 0};
    if(_model) {
        wxDataViewItemArray items;
        _model->GetChildren(wxDataViewItem(), items);
        ComputeCardSizes(items);
    }
}

void wxDataViewCardCtrl::UpdateScrollbars()
{
    int oldPos = GetScrollPos(wxVERTICAL);
    wxSize clientSize = GetClientSize();

    if(_cardSizes.empty() || clientSize.x <= 0 || clientSize.y <= 0) {
        SetScrollbar(wxVERTICAL, 0, 1, 1);
        return;
    }

    clientSize -= _marginSize;
    int cardPerRow = clientSize.x / (_maxSize.GetWidth() + _marginSize.GetWidth());
    if(cardPerRow <= 0) {
        cardPerRow = 1; // At least one card per row
    }
    int lineCount = _cardSizes.size() / cardPerRow;
    if(_cardSizes.size() % cardPerRow != 0) {
        lineCount++; // Add one more line if there are remaining cards
    }

    int cardPerColumn = clientSize.y / (_maxSize.GetHeight() + _marginSize.GetHeight());

    if(oldPos > lineCount) {
        oldPos = lineCount; // Clamp old position to max lines
    }

    SetScrollbar(wxVERTICAL, oldPos, cardPerColumn, lineCount);
}

