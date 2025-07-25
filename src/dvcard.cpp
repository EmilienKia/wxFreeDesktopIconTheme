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
END_EVENT_TABLE()

wxDataViewCardCtrl::wxDataViewCardCtrl() :
wxControl()
{
}

wxDataViewCardCtrl::wxDataViewCardCtrl(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name):
wxControl(parent, id, pos, size, style, wxDefaultValidator, name)
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
    if(!wxControl::Create(parent, id, pos, size, style, wxDefaultValidator, name)) {
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
            _model->DecRef();
        }
        _model = model;
        if(_model) {
            _model->IncRef();
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

        // Compute max size
        wxSize maxSize{0,0};
        for(unsigned int i = 0; i < count; ++i)
        {
            wxDataViewItem item = children[i];
            wxSize cardSize = _renderer->GetCardSize(*model, item);
            if(cardSize.GetWidth() > maxSize.GetWidth()) {
                maxSize.SetWidth(cardSize.GetWidth());
            }
            if(cardSize.GetHeight() > maxSize.GetHeight()) {
                maxSize.SetHeight(cardSize.GetHeight());
            }
        }

        wxSize margins{8, 8};
        wxPoint pos{margins.GetWidth(), margins.GetHeight()};;

        for(unsigned int i = 0; i < count; ++i)
        {
            wxDataViewItem item = children[i];
            dc.SetClippingRegion(pos, maxSize);
            _renderer->DrawCard(*model, item, dc, pos, maxSize);
            dc.DestroyClippingRegion();
            pos.x += maxSize.GetWidth() + margins.GetWidth();

            if(pos.x + maxSize.GetWidth() + margins.GetWidth() > clientSize.GetWidth()) {
                // If next card would overflow, move to next row
                pos.x = margins.GetWidth();
                pos.y += maxSize.GetHeight() + margins.GetHeight();
            }

            if(pos.y >= clientSize.GetHeight()) {
                // If next card would overflow vertically, stop drawing
                break;
            }
        }
    }
}
