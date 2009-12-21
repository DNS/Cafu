/////////////////////////////////////////////////////////////////////////////
// Name:        wx/docview.h
// Purpose:     Doc/View classes
// Author:      Julian Smart
// Modified by:
// Created:     01/02/97
// RCS-ID:      $Id: docview.h 59484 2009-03-11 16:11:54Z VZ $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_DOCH__
#define _WX_DOCH__

#include "wx/defs.h"

#if wxUSE_DOC_VIEW_ARCHITECTURE

#include "wx/list.h"
#include "wx/string.h"
#include "wx/frame.h"

#if wxUSE_PRINTING_ARCHITECTURE
    #include "wx/print.h"
#endif

class WXDLLIMPEXP_FWD_CORE wxWindow;
class WXDLLIMPEXP_FWD_CORE wxDocument;
class WXDLLIMPEXP_FWD_CORE wxView;
class WXDLLIMPEXP_FWD_CORE wxDocTemplate;
class WXDLLIMPEXP_FWD_CORE wxDocManager;
class WXDLLIMPEXP_FWD_CORE wxPrintInfo;
class WXDLLIMPEXP_FWD_CORE wxCommandProcessor;
class WXDLLIMPEXP_FWD_CORE wxFileHistory;
class WXDLLIMPEXP_FWD_BASE wxConfigBase;

class wxDocChildFrameAnyBase;

#if wxUSE_STD_IOSTREAM
  #include "wx/iosfwrap.h"
#else
  #include "wx/stream.h"
#endif

// Flags for wxDocManager (can be combined).
enum
{
    wxDOC_NEW    = 1,
    wxDOC_SILENT = 2
};

// Document template flags
enum
{
    wxTEMPLATE_VISIBLE = 1,
    wxTEMPLATE_INVISIBLE = 2,
    wxDEFAULT_TEMPLATE_FLAGS = wxTEMPLATE_VISIBLE
};

#define wxMAX_FILE_HISTORY 9

class WXDLLIMPEXP_CORE wxDocument : public wxEvtHandler
{
public:
    wxDocument(wxDocument *parent = NULL);
    virtual ~wxDocument();

    // accessors
    void SetFilename(const wxString& filename, bool notifyViews = false);
    wxString GetFilename() const { return m_documentFile; }

    void SetTitle(const wxString& title) { m_documentTitle = title; }
    wxString GetTitle() const { return m_documentTitle; }

    void SetDocumentName(const wxString& name) { m_documentTypeName = name; }
    wxString GetDocumentName() const { return m_documentTypeName; }

    // access the flag indicating whether this document had been already saved,
    // SetDocumentSaved() is only used internally, don't call it
    bool GetDocumentSaved() const { return m_savedYet; }
    void SetDocumentSaved(bool saved = true) { m_savedYet = saved; }

    // return true if the document hasn't been modified since the last time it
    // was saved (implying that it returns false if it was never saved, even if
    // the document is not modified)
    bool AlreadySaved() const { return !IsModified() && GetDocumentSaved(); }

    virtual bool Close();
    virtual bool Save();
    virtual bool SaveAs();
    virtual bool Revert();

#if wxUSE_STD_IOSTREAM
    virtual wxSTD ostream& SaveObject(wxSTD ostream& stream);
    virtual wxSTD istream& LoadObject(wxSTD istream& stream);
#else
    virtual wxOutputStream& SaveObject(wxOutputStream& stream);
    virtual wxInputStream& LoadObject(wxInputStream& stream);
#endif

    // Called by wxWidgets
    virtual bool OnSaveDocument(const wxString& filename);
    virtual bool OnOpenDocument(const wxString& filename);
    virtual bool OnNewDocument();
    virtual bool OnCloseDocument();

    // Prompts for saving if about to close a modified document. Returns true
    // if ok to close the document (may have saved in the meantime, or set
    // modified to false)
    virtual bool OnSaveModified();

    // if you override, remember to call the default
    // implementation (wxDocument::OnChangeFilename)
    virtual void OnChangeFilename(bool notifyViews);

    // Called by framework if created automatically by the default document
    // manager: gives document a chance to initialise and (usually) create a
    // view
    virtual bool OnCreate(const wxString& path, long flags);

    // By default, creates a base wxCommandProcessor.
    virtual wxCommandProcessor *OnCreateCommandProcessor();
    virtual wxCommandProcessor *GetCommandProcessor() const
        { return m_commandProcessor; }
    virtual void SetCommandProcessor(wxCommandProcessor *proc)
        { m_commandProcessor = proc; }

    // Called after a view is added or removed. The default implementation
    // deletes the document if this is there are no more views.
    virtual void OnChangedViewList();

    virtual bool DeleteContents();

    virtual bool Draw(wxDC&);
    virtual bool IsModified() const { return m_documentModified; }
    virtual void Modify(bool mod) { m_documentModified = mod; }

    virtual bool AddView(wxView *view);
    virtual bool RemoveView(wxView *view);
    wxList& GetViews() { return m_documentViews; }
    const wxList& GetViews() const { return m_documentViews; }
    wxView *GetFirstView() const;

    virtual void UpdateAllViews(wxView *sender = NULL, wxObject *hint = NULL);
    virtual void NotifyClosing();

    // Remove all views (because we're closing the document)
    virtual bool DeleteAllViews();

    // Other stuff
    virtual wxDocManager *GetDocumentManager() const;
    virtual wxDocTemplate *GetDocumentTemplate() const
        { return m_documentTemplate; }
    virtual void SetDocumentTemplate(wxDocTemplate *temp)
        { m_documentTemplate = temp; }

    // Get the document name to be shown to the user: the title if there is
    // any, otherwise the filename if the document was saved and, finally,
    // "unnamed" otherwise
    virtual wxString GetUserReadableName() const;

#if WXWIN_COMPATIBILITY_2_8
    // use GetUserReadableName() instead
    wxDEPRECATED_BUT_USED_INTERNALLY(
        virtual bool GetPrintableName(wxString& buf) const
    );
#endif // WXWIN_COMPATIBILITY_2_8

    // Returns a window that can be used as a parent for document-related
    // dialogs. Override if necessary.
    virtual wxWindow *GetDocumentWindow() const;

protected:
    wxList                m_documentViews;
    wxString              m_documentFile;
    wxString              m_documentTitle;
    wxString              m_documentTypeName;
    wxDocTemplate*        m_documentTemplate;
    bool                  m_documentModified;
    wxDocument*           m_documentParent;
    wxCommandProcessor*   m_commandProcessor;
    bool                  m_savedYet;

    // Called by OnSaveDocument and OnOpenDocument to implement standard
    // Save/Load behavior. Re-implement in derived class for custom
    // behavior.
    virtual bool DoSaveDocument(const wxString& file);
    virtual bool DoOpenDocument(const wxString& file);

    // the default implementation of GetUserReadableName()
    wxString DoGetUserReadableName() const;

private:
    DECLARE_ABSTRACT_CLASS(wxDocument)
    wxDECLARE_NO_COPY_CLASS(wxDocument);
};

class WXDLLIMPEXP_CORE wxView: public wxEvtHandler
{
public:
    wxView();
    virtual ~wxView();

    wxDocument *GetDocument() const { return m_viewDocument; }
    virtual void SetDocument(wxDocument *doc);

    wxString GetViewName() const { return m_viewTypeName; }
    void SetViewName(const wxString& name) { m_viewTypeName = name; }

    wxWindow *GetFrame() const { return m_viewFrame ; }
    void SetFrame(wxWindow *frame) { m_viewFrame = frame; }

    virtual void OnActivateView(bool activate,
                                wxView *activeView,
                                wxView *deactiveView);
    virtual void OnDraw(wxDC *dc) = 0;
    virtual void OnPrint(wxDC *dc, wxObject *info);
    virtual void OnUpdate(wxView *sender, wxObject *hint = NULL);
    virtual void OnClosingDocument() {}
    virtual void OnChangeFilename();

    // Called by framework if created automatically by the default document
    // manager class: gives view a chance to initialise
    virtual bool OnCreate(wxDocument *WXUNUSED(doc), long WXUNUSED(flags))
        { return true; }

    // Checks if the view is the last one for the document; if so, asks user
    // to confirm save data (if modified). If ok, deletes itself and returns
    // true.
    virtual bool Close(bool deleteWindow = true);

    // Override to do cleanup/veto close
    virtual bool OnClose(bool deleteWindow);

    // A view's window can call this to notify the view it is (in)active.
    // The function then notifies the document manager.
    virtual void Activate(bool activate);

    wxDocManager *GetDocumentManager() const
        { return m_viewDocument->GetDocumentManager(); }

#if wxUSE_PRINTING_ARCHITECTURE
    virtual wxPrintout *OnCreatePrintout();
#endif

    // implementation only
    // -------------------

    // set the associated frame, it is used to reset its view when we're
    // destroyed
    void SetDocChildFrame(wxDocChildFrameAnyBase *docChildFrame);

protected:
    // hook the document into event handlers chain here
    virtual bool TryBefore(wxEvent& event);

    wxDocument*       m_viewDocument;
    wxString          m_viewTypeName;
    wxWindow*         m_viewFrame;

    wxDocChildFrameAnyBase *m_docChildFrame;

private:
    DECLARE_ABSTRACT_CLASS(wxView)
    wxDECLARE_NO_COPY_CLASS(wxView);
};

// Represents user interface (and other) properties of documents and views
class WXDLLIMPEXP_CORE wxDocTemplate: public wxObject
{

friend class WXDLLIMPEXP_FWD_CORE wxDocManager;

public:
    // Associate document and view types. They're for identifying what view is
    // associated with what template/document type
    wxDocTemplate(wxDocManager *manager,
                  const wxString& descr,
                  const wxString& filter,
                  const wxString& dir,
                  const wxString& ext,
                  const wxString& docTypeName,
                  const wxString& viewTypeName,
                  wxClassInfo *docClassInfo = NULL,
                  wxClassInfo *viewClassInfo = NULL,
                  long flags = wxDEFAULT_TEMPLATE_FLAGS);

    virtual ~wxDocTemplate();

    // By default, these two member functions dynamically creates document and
    // view using dynamic instance construction. Override these if you need a
    // different method of construction.
    virtual wxDocument *CreateDocument(const wxString& path, long flags = 0);
    virtual wxView *CreateView(wxDocument *doc, long flags = 0);

    // Helper method for CreateDocument; also allows you to do your own document
    // creation
    virtual bool InitDocument(wxDocument* doc,
                              const wxString& path,
                              long flags = 0);

    wxString GetDefaultExtension() const { return m_defaultExt; }
    wxString GetDescription() const { return m_description; }
    wxString GetDirectory() const { return m_directory; }
    wxDocManager *GetDocumentManager() const { return m_documentManager; }
    void SetDocumentManager(wxDocManager *manager)
        { m_documentManager = manager; }
    wxString GetFileFilter() const { return m_fileFilter; }
    long GetFlags() const { return m_flags; }
    virtual wxString GetViewName() const { return m_viewTypeName; }
    virtual wxString GetDocumentName() const { return m_docTypeName; }

    void SetFileFilter(const wxString& filter) { m_fileFilter = filter; }
    void SetDirectory(const wxString& dir) { m_directory = dir; }
    void SetDescription(const wxString& descr) { m_description = descr; }
    void SetDefaultExtension(const wxString& ext) { m_defaultExt = ext; }
    void SetFlags(long flags) { m_flags = flags; }

    bool IsVisible() const { return (m_flags & wxTEMPLATE_VISIBLE) != 0; }

    wxClassInfo* GetDocClassInfo() const { return m_docClassInfo; }
    wxClassInfo* GetViewClassInfo() const { return m_viewClassInfo; }

    virtual bool FileMatchesTemplate(const wxString& path);

protected:
    long              m_flags;
    wxString          m_fileFilter;
    wxString          m_directory;
    wxString          m_description;
    wxString          m_defaultExt;
    wxString          m_docTypeName;
    wxString          m_viewTypeName;
    wxDocManager*     m_documentManager;

    // For dynamic creation of appropriate instances.
    wxClassInfo*      m_docClassInfo;
    wxClassInfo*      m_viewClassInfo;

    // Called by CreateDocument and CreateView to create the actual
    // document/view object.
    //
    // By default uses the ClassInfo provided to the constructor. Override
    // these functions to provide a different method of creation.
    virtual wxDocument *DoCreateDocument();
    virtual wxView *DoCreateView();

private:
    DECLARE_CLASS(wxDocTemplate)
    wxDECLARE_NO_COPY_CLASS(wxDocTemplate);
};

// One object of this class may be created in an application, to manage all
// the templates and documents.
class WXDLLIMPEXP_CORE wxDocManager: public wxEvtHandler
{
public:
    // NB: flags are unused, don't pass wxDOC_XXX to this ctor
    wxDocManager(long flags = 0, bool initialize = true);
    virtual ~wxDocManager();

    virtual bool Initialize();

    // Handlers for common user commands
    void OnFileClose(wxCommandEvent& event);
    void OnFileCloseAll(wxCommandEvent& event);
    void OnFileNew(wxCommandEvent& event);
    void OnFileOpen(wxCommandEvent& event);
    void OnFileRevert(wxCommandEvent& event);
    void OnFileSave(wxCommandEvent& event);
    void OnFileSaveAs(wxCommandEvent& event);
    void OnPrint(wxCommandEvent& event);
    void OnPreview(wxCommandEvent& event);
    void OnUndo(wxCommandEvent& event);
    void OnRedo(wxCommandEvent& event);

    // Handlers for UI update commands
    void OnUpdateFileOpen(wxUpdateUIEvent& event);
    void OnUpdateDisableIfNoDoc(wxUpdateUIEvent& event);
    void OnUpdateFileNew(wxUpdateUIEvent& event);
    void OnUpdateFileSave(wxUpdateUIEvent& event);
    void OnUpdateUndo(wxUpdateUIEvent& event);
    void OnUpdateRedo(wxUpdateUIEvent& event);

    // called when file format detection didn't work, can be overridden to do
    // something in this case
    virtual void OnOpenFileFailure() { }

    virtual wxDocument *CreateDocument(const wxString& path, long flags = 0);

    // wrapper around CreateDocument() with a more clear name
    wxDocument *CreateNewDocument()
        { return CreateDocument(wxString(), wxDOC_NEW); }

    virtual wxView *CreateView(wxDocument *doc, long flags = 0);
    virtual void DeleteTemplate(wxDocTemplate *temp, long flags = 0);
    virtual bool FlushDoc(wxDocument *doc);
    virtual wxDocTemplate *MatchTemplate(const wxString& path);
    virtual wxDocTemplate *SelectDocumentPath(wxDocTemplate **templates,
            int noTemplates, wxString& path, long flags, bool save = false);
    virtual wxDocTemplate *SelectDocumentType(wxDocTemplate **templates,
            int noTemplates, bool sort = false);
    virtual wxDocTemplate *SelectViewType(wxDocTemplate **templates,
            int noTemplates, bool sort = false);
    virtual wxDocTemplate *FindTemplateForPath(const wxString& path);

    void AssociateTemplate(wxDocTemplate *temp);
    void DisassociateTemplate(wxDocTemplate *temp);

    wxDocument *GetCurrentDocument() const;

    void SetMaxDocsOpen(int n) { m_maxDocsOpen = n; }
    int GetMaxDocsOpen() const { return m_maxDocsOpen; }

    // Add and remove a document from the manager's list
    void AddDocument(wxDocument *doc);
    void RemoveDocument(wxDocument *doc);

    // closes all currently open documents
    bool CloseDocuments(bool force = true);

    // closes the specified document
    bool CloseDocument(wxDocument* doc, bool force = false);

    // Clear remaining documents and templates
    bool Clear(bool force = true);

    // Views or windows should inform the document manager
    // when a view is going in or out of focus
    virtual void ActivateView(wxView *view, bool activate = true);
    virtual wxView *GetCurrentView() const { return m_currentView; }

    wxList& GetDocuments() { return m_docs; }
    wxList& GetTemplates() { return m_templates; }

    // Return the default name for a new document (by default returns strings
    // in the form "unnamed <counter>" but can be overridden)
    virtual wxString MakeNewDocumentName();

    // Make a frame title (override this to do something different)
    virtual wxString MakeFrameTitle(wxDocument* doc);

    virtual wxFileHistory *OnCreateFileHistory();
    virtual wxFileHistory *GetFileHistory() const { return m_fileHistory; }

    // File history management
    virtual void AddFileToHistory(const wxString& file);
    virtual void RemoveFileFromHistory(size_t i);
    virtual size_t GetHistoryFilesCount() const;
    virtual wxString GetHistoryFile(size_t i) const;
    virtual void FileHistoryUseMenu(wxMenu *menu);
    virtual void FileHistoryRemoveMenu(wxMenu *menu);
#if wxUSE_CONFIG
    virtual void FileHistoryLoad(const wxConfigBase& config);
    virtual void FileHistorySave(wxConfigBase& config);
#endif // wxUSE_CONFIG

    virtual void FileHistoryAddFilesToMenu();
    virtual void FileHistoryAddFilesToMenu(wxMenu* menu);

    wxString GetLastDirectory() const;
    void SetLastDirectory(const wxString& dir) { m_lastDirectory = dir; }

    // Get the current document manager
    static wxDocManager* GetDocumentManager() { return sm_docManager; }

#if WXWIN_COMPATIBILITY_2_8
    // deprecated, override GetDefaultName() instead
    wxDEPRECATED_BUT_USED_INTERNALLY(
        virtual bool MakeDefaultName(wxString& buf)
    );
#endif

#if WXWIN_COMPATIBILITY_2_6
    // deprecated, use GetHistoryFilesCount() instead
    wxDEPRECATED( size_t GetNoHistoryFiles() const );
#endif // WXWIN_COMPATIBILITY_2_6

protected:
    // hook the currently active view into event handlers chain here
    virtual bool TryBefore(wxEvent& event);

    // return the command processor for the current document, if any
    wxCommandProcessor *GetCurrentCommandProcessor() const;

    // this method tries to find an active view harder than GetCurrentView():
    // if the latter is NULL, it also checks if we don't have just a single
    // view and returns it then
    wxView *GetActiveView() const;


    int               m_defaultDocumentNameCounter;
    int               m_maxDocsOpen;
    wxList            m_docs;
    wxList            m_templates;
    wxView*           m_currentView;
    wxFileHistory*    m_fileHistory;
    wxString          m_lastDirectory;
    static wxDocManager* sm_docManager;

    DECLARE_EVENT_TABLE()
    DECLARE_DYNAMIC_CLASS(wxDocManager)
    wxDECLARE_NO_COPY_CLASS(wxDocManager);
};

#if WXWIN_COMPATIBILITY_2_6
inline size_t wxDocManager::GetNoHistoryFiles() const
{
    return GetHistoryFilesCount();
}
#endif // WXWIN_COMPATIBILITY_2_6

// ----------------------------------------------------------------------------
// Base class for child frames -- this is what wxView renders itself into
//
// Notice that this is a mix-in class so it doesn't derive from wxWindow, only
// wxDocChildFrameAny does
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_CORE wxDocChildFrameAnyBase
{
public:
    wxDocChildFrameAnyBase(wxDocument *doc, wxView *view, wxWindow *win)
        : m_win(win)
    {
        m_childDocument = doc;
        m_childView = view;

        if ( view )
            view->SetDocChildFrame(this);
    }

    ~wxDocChildFrameAnyBase()
    {
        // prevent the view from deleting us if we're being deleted directly
        // (and not via Close() + Destroy())
        if ( m_childView )
            m_childView->SetDocChildFrame(NULL);
    }

    wxDocument *GetDocument() const { return m_childDocument; }
    wxView *GetView() const { return m_childView; }
    void SetDocument(wxDocument *doc) { m_childDocument = doc; }
    void SetView(wxView *view) { m_childView = view; }

    wxWindow *GetWindow() const { return m_win; }

protected:
    // we're not a wxEvtHandler but we provide this wxEvtHandler-like function
    // which is called from TryBefore() of the derived classes to give our view
    // a chance to process the message before the frame event handlers are used
    bool TryProcessEvent(wxEvent& event)
    {
        return m_childView && m_childView->ProcessEventHere(event);
    }

    // called from EVT_CLOSE handler in the frame: check if we can close and do
    // cleanup if so; veto the event otherwise
    bool CloseView(wxCloseEvent& event);


    wxDocument*       m_childDocument;
    wxView*           m_childView;

    // the associated window: having it here is not terribly elegant but it
    // allows us to avoid having any virtual functions in this class
    wxWindow * const m_win;


    wxDECLARE_NO_COPY_CLASS(wxDocChildFrameAnyBase);
};

// ----------------------------------------------------------------------------
// Template implementing child frame concept using the given wxFrame-like class
//
// This is used to define wxDocChildFrame and wxDocMDIChildFrame: ChildFrame is
// a wxFrame or wxMDIChildFrame (although in theory it could be any wxWindow-
// derived class as long as it provided a ctor with the same signature as
// wxFrame and OnActivate() method) and ParentFrame is either wxFrame or
// wxMDIParentFrame.
// ----------------------------------------------------------------------------

template <class ChildFrame, class ParentFrame>
class WXDLLIMPEXP_CORE wxDocChildFrameAny : public ChildFrame,
                                            public wxDocChildFrameAnyBase
{
public:
    typedef ChildFrame BaseClass;

    // ctor for a frame showing the given view of the specified document
    wxDocChildFrameAny(wxDocument *doc,
                       wxView *view,
                       ParentFrame *parent,
                       wxWindowID id,
                       const wxString& title,
                       const wxPoint& pos = wxDefaultPosition,
                       const wxSize& size = wxDefaultSize,
                       long style = wxDEFAULT_FRAME_STYLE,
                       const wxString& name = wxFrameNameStr)
        : BaseClass(parent, id, title, pos, size, style, name),
          wxDocChildFrameAnyBase(doc, view, this)
    {
        this->Connect(wxEVT_ACTIVATE,
                      wxActivateEventHandler(wxDocChildFrameAny::OnActivate));
        this->Connect(wxEVT_CLOSE_WINDOW,
                      wxCloseEventHandler(wxDocChildFrameAny::OnCloseWindow));
    }

    virtual bool Destroy()
    {
        // FIXME: why exactly do we do this? to avoid activation events during
        //        destructions maybe?
        m_childView = NULL;
        return BaseClass::Destroy();
    }

protected:
    // hook the child view into event handlers chain here
    virtual bool TryBefore(wxEvent& event)
    {
        return TryProcessEvent(event) || BaseClass::TryBefore(event);
    }

private:
    void OnActivate(wxActivateEvent& event)
    {
        BaseClass::OnActivate(event);

        if ( m_childView )
            m_childView->Activate(event.GetActive());
    }

    void OnCloseWindow(wxCloseEvent& event)
    {
        if ( CloseView(event) )
            Destroy();
        //else: vetoed
    }

    wxDECLARE_NO_COPY_TEMPLATE_CLASS_2(wxDocChildFrameAny,
                                        ChildFrame, ParentFrame);
};

// ----------------------------------------------------------------------------
// A default child frame: we need to define it as a class just for wxRTTI,
// otherwise we could simply typedef it
// ----------------------------------------------------------------------------

#ifdef __VISUALC6__
    // "non dll-interface class 'wxDocChildFrameAny<>' used as base interface
    // for dll-interface class 'wxDocChildFrame'" -- this is bogus as the
    // template will be DLL-exported but only once it is used as base class
    // here!
    #pragma warning (disable:4275)
#endif

typedef wxDocChildFrameAny<wxFrame, wxFrame> wxDocChildFrameBase;

class WXDLLIMPEXP_CORE wxDocChildFrame : public wxDocChildFrameBase
{
public:
    wxDocChildFrame(wxDocument *doc,
                    wxView *view,
                    wxFrame *parent,
                    wxWindowID id,
                    const wxString& title,
                    const wxPoint& pos = wxDefaultPosition,
                    const wxSize& size = wxDefaultSize,
                    long style = wxDEFAULT_FRAME_STYLE,
                    const wxString& name = wxFrameNameStr)
        : wxDocChildFrameBase(doc, view,
                              parent, id, title, pos, size, style, name)
    {
    }

private:
    DECLARE_CLASS(wxDocChildFrame)
    wxDECLARE_NO_COPY_CLASS(wxDocChildFrame);
};

#ifdef __VISUALC6__
    #pragma warning (default:4275)
#endif

// ----------------------------------------------------------------------------
// A default parent frame
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_CORE wxDocParentFrame : public wxFrame
{
public:
    wxDocParentFrame();
    wxDocParentFrame(wxDocManager *manager,
                     wxFrame *frame,
                     wxWindowID id,
                     const wxString& title,
                     const wxPoint& pos = wxDefaultPosition,
                     const wxSize& size = wxDefaultSize,
                     long style = wxDEFAULT_FRAME_STYLE,
                     const wxString& name = wxFrameNameStr);

    bool Create(wxDocManager *manager,
                wxFrame *frame,
                wxWindowID id,
                const wxString& title,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxDEFAULT_FRAME_STYLE,
                const wxString& name = wxFrameNameStr);

    wxDocManager *GetDocumentManager() const { return m_docManager; }

    void OnExit(wxCommandEvent& event);
    void OnMRUFile(wxCommandEvent& event);
    void OnCloseWindow(wxCloseEvent& event);

protected:
    // hook the document manager into event handling chain here
    virtual bool TryBefore(wxEvent& event);

    wxDocManager *m_docManager;

private:
    typedef wxFrame base_type;
    DECLARE_CLASS(wxDocParentFrame)
    DECLARE_EVENT_TABLE()
    wxDECLARE_NO_COPY_CLASS(wxDocParentFrame);
};

// ----------------------------------------------------------------------------
// Provide simple default printing facilities
// ----------------------------------------------------------------------------

#if wxUSE_PRINTING_ARCHITECTURE
class WXDLLIMPEXP_CORE wxDocPrintout : public wxPrintout
{
public:
    wxDocPrintout(wxView *view = NULL, const wxString& title = wxT("Printout"));

    // implement wxPrintout methods
    virtual bool OnPrintPage(int page);
    virtual bool HasPage(int page);
    virtual bool OnBeginDocument(int startPage, int endPage);
    virtual void GetPageInfo(int *minPage, int *maxPage,
                             int *selPageFrom, int *selPageTo);

    virtual wxView *GetView() { return m_printoutView; }

protected:
    wxView*       m_printoutView;

private:
    DECLARE_DYNAMIC_CLASS(wxDocPrintout)
    wxDECLARE_NO_COPY_CLASS(wxDocPrintout);
};
#endif // wxUSE_PRINTING_ARCHITECTURE

// ----------------------------------------------------------------------------
// File history management
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_CORE wxFileHistory : public wxObject
{
public:
    wxFileHistory(size_t maxFiles = 9, wxWindowID idBase = wxID_FILE1);

    // Operations
    virtual void AddFileToHistory(const wxString& file);
    virtual void RemoveFileFromHistory(size_t i);
    virtual int GetMaxFiles() const { return (int)m_fileMaxFiles; }
    virtual void UseMenu(wxMenu *menu);

    // Remove menu from the list (MDI child may be closing)
    virtual void RemoveMenu(wxMenu *menu);

#if wxUSE_CONFIG
    virtual void Load(const wxConfigBase& config);
    virtual void Save(wxConfigBase& config);
#endif // wxUSE_CONFIG

    virtual void AddFilesToMenu();
    virtual void AddFilesToMenu(wxMenu* menu); // Single menu

    // Accessors
    virtual wxString GetHistoryFile(size_t i) const { return m_fileHistory[i]; }
    virtual size_t GetCount() const { return m_fileHistory.GetCount(); }

    const wxList& GetMenus() const { return m_fileMenus; }

    // Set/get base id
    void SetBaseId(wxWindowID baseId) { m_idBase = baseId; }
    wxWindowID GetBaseId() const { return m_idBase; }

#if WXWIN_COMPATIBILITY_2_6
    // deprecated, use GetCount() instead
    wxDEPRECATED( size_t GetNoHistoryFiles() const );
#endif // WXWIN_COMPATIBILITY_2_6

protected:
    // Last n files
    wxArrayString     m_fileHistory;

    // Menus to maintain (may need several for an MDI app)
    wxList            m_fileMenus;

    // Max files to maintain
    size_t            m_fileMaxFiles;

private:
    // The ID of the first history menu item (Doesn't have to be wxID_FILE1)
    wxWindowID m_idBase;

    DECLARE_DYNAMIC_CLASS(wxFileHistory)
    wxDECLARE_NO_COPY_CLASS(wxFileHistory);
};

#if WXWIN_COMPATIBILITY_2_6
inline size_t wxFileHistory::GetNoHistoryFiles() const
{
    return m_fileHistory.GetCount();
}
#endif // WXWIN_COMPATIBILITY_2_6

// For compatibility with existing file formats:
// converts from/to a stream to/from a temporary file.
#if wxUSE_STD_IOSTREAM
bool WXDLLIMPEXP_CORE
wxTransferFileToStream(const wxString& filename, wxSTD ostream& stream);
bool WXDLLIMPEXP_CORE
wxTransferStreamToFile(wxSTD istream& stream, const wxString& filename);
#else
bool WXDLLIMPEXP_CORE
wxTransferFileToStream(const wxString& filename, wxOutputStream& stream);
bool WXDLLIMPEXP_CORE
wxTransferStreamToFile(wxInputStream& stream, const wxString& filename);
#endif // wxUSE_STD_IOSTREAM


// these flags are not used anywhere by wxWidgets and kept only for an unlikely
// case of existing user code using them for its own purposes
#ifdef WXWIN_COMPATIBILITY_2_8
enum
{
    wxDOC_SDI = 1,
    wxDOC_MDI,
    wxDEFAULT_DOCMAN_FLAGS = wxDOC_SDI
};
#endif // WXWIN_COMPATIBILITY_2_8

#endif // wxUSE_DOC_VIEW_ARCHITECTURE

#endif // _WX_DOCH__
