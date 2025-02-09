#include "JsWindow.h"
#include "Util.h"


namespace duijs {

JsWindow::JsWindow(Context* context, Value& this_obj)
	:context_(context),this_(this_obj)
{
	Value mgr = WeakClass<CPaintManagerUI>::ToJs(*context, m_pm.get_weak_ptr<CPaintManagerUI>());
	this_.SetProperty("manager", mgr);
}

JsWindow::~JsWindow() {
	this_.SetOpaque(nullptr);
	this_ = undefined_value;
}

LPCTSTR JsWindow::GetWindowClassName(void) const {
	return _T("DUIJS");
}

CDuiString JsWindow::GetSkinFile() {
	Value result = this_.Invoke("getSkinFile");
	if (result.IsException()) {
		context_->DumpError();
	}
	auto str = result.ToString();
	return CDuiString(str.str(),str.len());
}

void JsWindow::InitResource() {
	if (this_.HasProperty("initResource")) {
		Value result = this_.Invoke("initResource");
		if (result.IsException()) {
			context_->DumpError();
		}
	}
}

void JsWindow::InitWindow() {
	if (this_.HasProperty("initWindow")) {
		Value result = this_.Invoke("initWindow");
		if (result.IsException()) {
			context_->DumpError();
		}
	}
}

void JsWindow::OnFinalMessage(HWND hWnd) {
	delete this;
}

void JsWindow::Notify(TNotifyUI& msg) {
	//�����¼�
	LPCTSTR funcName = msg.pSender->GetCustomAttribute(msg.sType);
	if (funcName) {
		std::string name = Wide2UTF8(funcName);

		auto find = name.find(".");
		if (find != std::string::npos) {
			std::string prostr = name.substr(0,find);
			auto object = this_.GetProperty(prostr.c_str());
			if (object.IsObject()) {
				std::string funcstr = name.substr(find+1);
				if (object.HasProperty(funcstr.c_str())) {

					Value result = object.Invoke(funcstr.c_str(),
						toValue(*context_, msg.pSender),
						toValue(*context_, (uint32_t)msg.wParam),
						toValue(*context_, (uint32_t)msg.lParam));

					if (result.IsException()) {
						context_->DumpError();
					}
					return;
				} else {
					printf("no property %s", funcstr.c_str());
				}
			} else {
				printf("no object %s", prostr.c_str());
			}

		} else if (this_.HasProperty(name.c_str())) {

			Value result = this_.Invoke(name.c_str(),
				toValue(*context_,msg.pSender),
				toValue(*context_,(uint32_t)msg.wParam), 
				toValue(*context_,(uint32_t)msg.lParam));

			if (result.IsException()) {
				context_->DumpError();
			}
			return;
		} else {
			printf("no func %s", name.c_str());
		}
	}
	WindowImplBase::Notify(msg);
}


CDuiString JsWindow::GetSkinType() {
	if (this_.HasProperty("getSkinType")) {
		Value result = this_.Invoke("getSkinType");
		if (result.IsException()) {
			context_->DumpError();
		} else {
			auto str = result.ToString();
			return CDuiString(str.str(), str.len());
		}
	}
	return _T("");
}

LPCTSTR JsWindow::GetManagerName() {
	if (this_.HasProperty("getManagerName")) {
		Value result = this_.Invoke("getManagerName");
		if (result.IsException()) {
			context_->DumpError();
		}
		else {
			auto str = result.ToString();
			return CDuiString(str.str(), str.len());
		}
	}
	return NULL;
}

CControlUI* JsWindow::CreateControl(LPCTSTR pstrClass) {
	if (this_.HasProperty("createControl")) {
		Value result = this_.Invoke("createControl",toValue(*context_,pstrClass));
		if (result.IsException()) {
			context_->DumpError();
		} else {
			return toControl(result);
		}
	}
	return NULL;
}

CDuiString* JsWindow::QueryControlText(LPCTSTR lpstrId, LPCTSTR lpstrType) {
	if (this_.HasProperty("queryControlText")) {
		Value result = this_.Invoke("queryControlText", 
			toValue(*context_, lpstrId), toValue(*context_, lpstrType));
		if (result.IsException()) {
			context_->DumpError();
		} else {
			auto str = result.ToString();
			return new CDuiString(str.str(), str.len());
		}
	}
	return NULL;
}

LRESULT JsWindow::OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	if (this_.HasProperty("onClose")) {
		Value result = this_.Invoke("onClose");
		if (result.IsException()) {
			context_->DumpError();
		}
	}
	return WindowImplBase::OnClose(uMsg,wParam,lParam,bHandled);
}

LRESULT JsWindow::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	if (this_.HasProperty("onDestroy")) {
		Value result = this_.Invoke("onDestroy");
		if (result.IsException()) {
			context_->DumpError();
		}
	}
	return WindowImplBase::OnDestroy(uMsg, wParam, lParam, bHandled);
}

LRESULT JsWindow::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	if (this_.HasProperty("onSize")) {
		UINT width = LOWORD(lParam);
		UINT height = HIWORD(lParam);

		Value result = this_.Invoke("onSize",context_->NewUint32(width),
			context_->NewUint32(height));
		if (result.IsException()) {
			context_->DumpError();
		}
	}
	return WindowImplBase::OnSize(uMsg, wParam, lParam, bHandled);
}


LRESULT JsWindow::OnChar(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {

	if (this_.HasProperty("onChar")) {
		Value result = this_.Invoke("onChar",
			context_->NewUint32(wParam),
			context_->NewUint32(lParam));
		if (result.IsException()) {
			context_->DumpError();
		}
	}
	return WindowImplBase::OnChar(uMsg, wParam, lParam, bHandled);
}

LRESULT JsWindow::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	if (this_.HasProperty("onKeyDown")) {

		WORD vkCode = LOWORD(wParam);
		Value result = this_.Invoke("onKeyDown", 
			context_->NewUint32(vkCode),
			context_->NewUint32(lParam));
		if (result.IsException()) {
			context_->DumpError();
		}
	}
	return WindowImplBase::OnKeyDown(uMsg, wParam, lParam, bHandled);
}

LRESULT JsWindow::OnKeyUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	if (this_.HasProperty("onKeyUp")) {

		WORD vkCode = LOWORD(wParam);
		Value result = this_.Invoke("onKeyUp",
			context_->NewUint32(vkCode),
			context_->NewUint32(lParam));
		if (result.IsException()) {
			context_->DumpError();
		}
	}
	return WindowImplBase::OnKeyUp(uMsg, wParam, lParam, bHandled);
}

LRESULT JsWindow::OnKillFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {

	if (this_.HasProperty("onKillFocus")) {
		Value result = this_.Invoke("onKillFocus");
		if (result.IsException()) {
			context_->DumpError();
		}
	}
	return WindowImplBase::OnKillFocus(uMsg, wParam, lParam, bHandled);
}
LRESULT JsWindow::OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {

	if (this_.HasProperty("onSetFocus")) {
		Value result = this_.Invoke("onSetFocus");
		if (result.IsException()) {
			context_->DumpError();
		}
	}
	return WindowImplBase::OnSetFocus(uMsg, wParam, lParam, bHandled);
}


LRESULT JsWindow::OnLButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	if (this_.HasProperty("onLButtonDown")) {

		int xPos = GET_X_LPARAM(lParam);
		int yPos = GET_Y_LPARAM(lParam);

		Value result = this_.Invoke("onLButtonDown",
			context_->NewUint32(xPos), context_->NewUint32(yPos), 
			context_->NewUint32(wParam));
		if (result.IsException()) {
			context_->DumpError();
		}
	}
	return WindowImplBase::OnLButtonDown(uMsg, wParam, lParam, bHandled);

}
LRESULT JsWindow::OnLButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {

	if (this_.HasProperty("onLButtonUp")) {
		int xPos = GET_X_LPARAM(lParam);
		int yPos = GET_Y_LPARAM(lParam);

		Value result = this_.Invoke("onLButtonUp",
			context_->NewUint32(xPos), context_->NewUint32(yPos),
			context_->NewUint32(wParam));
		if (result.IsException()) {
			context_->DumpError();
		}
	}
	return WindowImplBase::OnLButtonUp(uMsg, wParam, lParam, bHandled);
}

LRESULT JsWindow::OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {

	if (this_.HasProperty("onMouseMove")) {
		int xPos = GET_X_LPARAM(lParam);
		int yPos = GET_Y_LPARAM(lParam);

		Value result = this_.Invoke("onMouseMove",
			context_->NewUint32(xPos), context_->NewUint32(yPos),
			context_->NewUint32(wParam));
		if (result.IsException()) {
			context_->DumpError();
		}
	}
	return WindowImplBase::OnMouseMove(uMsg, wParam, lParam, bHandled);
}

#if defined(WIN32) && !defined(UNDER_CE)

LRESULT JsWindow::OnMouseWheel(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	if (this_.HasProperty("onMouseWheel")) {
		int xPos = GET_X_LPARAM(lParam);
		int yPos = GET_Y_LPARAM(lParam);
		int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
		Value result = this_.Invoke("onMouseWheel",
			context_->NewUint32(xPos), context_->NewUint32(yPos),
			context_->NewUint32(zDelta),
			context_->NewUint32(GET_KEYSTATE_WPARAM(wParam)));
		if (result.IsException()) {
			context_->DumpError();
		}
	}
	return WindowImplBase::OnMouseWheel(uMsg, wParam, lParam, bHandled);
}

LRESULT JsWindow::OnMouseHover(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	if (this_.HasProperty("onMouseHover")) {
		int xPos = GET_X_LPARAM(lParam);
		int yPos = GET_Y_LPARAM(lParam);
		
		Value result = this_.Invoke("onMouseHover",
			context_->NewUint32(xPos), context_->NewUint32(yPos),
			context_->NewUint32(wParam));
		if (result.IsException()) {
			context_->DumpError();
		}
	}
	return WindowImplBase::OnMouseHover(uMsg, wParam, lParam, bHandled);
}
#endif

LRESULT JsWindow::HandleCustomMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	if (uMsg == WM_DROPFILES) {
		if (this_.HasProperty("onDropFiles")) {
			HDROP hDrop = reinterpret_cast<HDROP>(wParam);
			TCHAR filename[MAX_PATH];

			auto list = context_->NewArray();

			UINT count = DragQueryFile(hDrop, -1, NULL, 0);
			for (UINT i = 0; i < count; ++i) {
				UINT size = DragQueryFile(hDrop, i, filename, MAX_PATH);
				if (size > 0) {
					filename[size] = '\0';
					list.SetProperty(i, toValue(*context_, filename));
				}
			}
			DragFinish(hDrop);
			Value result = this_.Invoke("onDropFiles", list);
			if (result.IsException()) {
				context_->DumpError();
			}
			return 0;
		}
	} else if (this_.HasProperty("handleCustomMessage")) {
		Value result = this_.Invoke("handleCustomMessage", context_->NewUint32(uMsg),
			context_->NewUint32(wParam), context_->NewUint32(lParam));
		if (result.IsException()) {
			context_->DumpError();
		}
	}
	return WindowImplBase::HandleCustomMessage(uMsg, wParam, lParam, bHandled);
}

void JsWindow::SetTop()
{
	HWND hForeWnd = NULL;
	HWND hWnd = m_hWnd;
	DWORD dwForeID;
	DWORD dwCurID;

	hForeWnd = GetForegroundWindow();
	dwCurID = GetCurrentThreadId();
	dwForeID = GetWindowThreadProcessId(hForeWnd, NULL);
	AttachThreadInput(dwCurID, dwForeID, TRUE);
	if (IsIconic(hWnd) || IsWindowVisible(hWnd) == FALSE)
		::ShowWindow(hWnd, SW_SHOWNORMAL);

	if ((GetWindowLong(m_hWnd, GWL_EXSTYLE) & WS_EX_TOPMOST) == 0)
	{
		SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
		SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
	}

	SetForegroundWindow(hWnd);
	AttachThreadInput(dwCurID, dwForeID, FALSE);
}

//////////////////////////////////////////////////////////////////////

static WeakPtr<JsWindow> newWindow(Context& context,Value& this_obj, ArgList& args) {
	JsWindow* wnd = new JsWindow(&context, this_obj);
	return wnd->get_weak_ptr<JsWindow>();
}

static Value createWindow(JsWindow* pThis, Context& context, ArgList& args) {
	HWND hWnd = NULL;
	if (args[0].IsObject()) {
		auto parent = WeakClass<JsWindow>::ToC(args[0]);
		auto name = args[1].ToString();
		hWnd = pThis->CreateDuiWindow(parent ? parent->GetHWND() : NULL,
			CDuiString(name.str(), name.len()), args[2].ToUint32(), args[3].ToUint32());
	}
	else {
		auto name = args[0].ToString();
		hWnd = pThis->CreateDuiWindow(NULL,
			CDuiString(name.str(), name.len()), args[1].ToUint32(), args[2].ToUint32());
	}
	return context.NewBool(hWnd != NULL);
}

static Value closeWindow(JsWindow* pThis, Context& context, ArgList& args) {
	if(args.size() == 1)
		pThis->Close(args[0].ToUint32());
	else
		pThis->Close();
	return undefined_value;
}

static Value showWindow(JsWindow* pThis, Context& context, ArgList& args) {
	if (args.size() == 2)
		pThis->ShowWindow(args[0].ToBool(), args[1].ToBool());
	else if (args.size() == 1)
		pThis->ShowWindow(args[0].ToBool());
	else
		pThis->ShowWindow();
	return undefined_value;
}

static Value setTop(JsWindow* pThis, Context& context, ArgList& args) {

	pThis->SetTop();
	return undefined_value;
}


static Value centerWindow(JsWindow* pThis, Context& context, ArgList& args) {

	pThis->CenterWindow();
	return undefined_value;
}

static Value showModal(JsWindow* pThis, Context& context, ArgList& args) {
	UINT rslt = pThis->ShowModal();
	return context.NewUint32(rslt);
}

static Value setIcon(JsWindow* pThis, Context& context, ArgList& args) {
	pThis->SetIcon(args[0].ToUint32());
	return undefined_value;
}


static Value resizeClient(JsWindow* pThis, Context& context, ArgList& args) {
	pThis->ResizeClient(args[0].ToInt32(), args[1].ToInt32());
	return undefined_value;
}

static Value sendMessage(JsWindow* pThis, Context& context, ArgList& args) {
	LRESULT result = pThis->SendMessage(args[0].ToInt32(), args[1].ToInt32(), args[2].ToInt32());
	return context.NewInt64(result);
}

static Value postMessage(JsWindow* pThis, Context& context, ArgList& args) {
	LRESULT result = pThis->PostMessage(args[0].ToInt32(), args[1].ToInt32(), args[2].ToInt32());
	return context.NewInt64(result);
}


static Value dragAcceptFiles(JsWindow* pThis, Context& context, ArgList& args) {
	DragAcceptFiles(*pThis, args[0].ToBool());
	return undefined_value;
}



void RegisterWindow(qjs::Module* module) {
	auto window = module->ExportWeakClass<JsWindow>("Window");
	window.Init();
	window.AddCtor2<newWindow>();
	window.AddFunc<createWindow>("create");
	window.AddFunc<closeWindow>("close");
	window.AddFunc<showWindow>("showWindow");
	window.AddFunc<setTop>("setTop");
	window.AddFunc<centerWindow>("centerWindow");
	window.AddFunc<showModal>("showModal");
	window.AddFunc<setIcon>("setIcon");
	window.AddFunc<resizeClient>("resizeClient");
	window.AddFunc<sendMessage>("sendMessage");
	window.AddFunc<postMessage>("postMessage");
	window.AddFunc<dragAcceptFiles>("dragAcceptFiles");


}

}//namespace


