#include "qjs.h"
#include <stdarg.h>

namespace qjs {

const Value undefined_value(nullptr, JS_UNDEFINED);
const Value null_value(nullptr, JS_NULL);
const Value true_value(nullptr, JS_TRUE);
const Value false_value(nullptr, JS_FALSE);
const Value exception_value(nullptr, JS_EXCEPTION);
const Value uninit_value(nullptr, JS_UNINITIALIZED);

static void js_dump_obj(JSContext* ctx,std::string& msg, JSValueConst val)
{
	const char* str;

	str = JS_ToCString(ctx, val);
	if (str) {
		msg.append(str);
		JS_FreeCString(ctx, str);
	}
	else {
		msg.append("[exception]");
	}
}

static std::string dump_error(JSContext* ctx, JSValueConst exception_val)
{
	std::string msg;
	JSValue val;
	bool is_error = JS_IsError(ctx, exception_val);
	js_dump_obj(ctx, msg, exception_val);
	if (is_error) {
		val = JS_GetPropertyStr(ctx, exception_val, "stack");
		if (!JS_IsUndefined(val)) {
			js_dump_obj(ctx, msg, val);
		}
		JS_FreeValue(ctx, val);
	}
	return msg;
}


void Runtime::PromiseRejectionTracker(JSContext* ctx, JSValueConst promise,
	JSValueConst reason,
	JS_BOOL is_handled, void* opaque) {
	if (!is_handled) {
		Context* context = Context::get(ctx);
		context->Log(dump_error(ctx, reason));
	}
}




Module::Module(JSContext* ctx, const char* name)
	:context_(ctx), module_(nullptr)
{
	module_ = JS_NewCModule(ctx, name, Module::OnInit);
}

Module::~Module() {

}

bool Module::Export(const char* name, const Value& value) {
	int rslt = JS_AddModuleExport(context_, module_, name);
	exports_.insert({ name,value });
	return rslt == 0;
}

bool Module::Export(const char* name, JSValue value) {
	int rslt = JS_AddModuleExport(context_, module_, name);
	exports_.insert({ name,Value(context_,std::move(value)) });
	return rslt == 0;
}

Value Module::GetImportMeta() {
	return Value(context_, JS_GetImportMeta(context_, module_));
}

int Module::OnInit(JSContext* ctx, JSModuleDef* m) {
	Context* context = Context::get(ctx);
	if (!context) {
		return -1;
	}

	auto module = context->GetModule(m);
	if (!module) {
		return -1;
	}

	//����value
	for (auto& itr : module->exports_) {
		JS_SetModuleExport(ctx, m, itr.first.c_str(),
			itr.second.CopyValue());
	}
	return 0;
}

Context::Context(Runtime* runtime)
	:context_(JS_NewContext(runtime->runtime_))
{
	Init(0, nullptr);
}


Context::Context(Runtime* runtime, int argc, char** argv)
	:context_(JS_NewContext(runtime->runtime_))
{
	Init(argc, argv);
}


Context::Context(JSContext* context)
	:context_(JS_DupContext(context))
{
}

Context::~Context() {
	modules_.clear();
	if (context_) {
		JS_FreeContext(context_);
	}
}


void Context::AddClassId(JSClassID classid, JSClassID parent_classid) {
	if (parent_classid)
		class_ids_.insert(std::make_pair(classid, parent_classid));
}

JSClassID Context::GetParentClassId(JSClassID classid) {
	auto find = class_ids_.find(classid);
	if (find != class_ids_.end()) {
		return find->second;
	}
	return 0;
}

void Context::Init(int argc, char** argv) {
	JS_SetContextOpaque(context_, this);
	//js_init_module_std(context_, "std");
	//js_init_module_os(context_, "os");
	js_std_add_helpers(context_, argc, argv);
	JS_AddIntrinsicBigFloat(context_);
	JS_AddIntrinsicBigDecimal(context_);
}


void Context::ExecuteJobs() {
	JSContext* ctx1;
	int err;

	for (;;) {
		err = JS_ExecutePendingJob(JS_GetRuntime(context_), &ctx1);
		if (err <= 0) {
			if (err < 0) {
				DumpError();
			}
			break;
		}
	}
}

Module* Context::NewModule(const char* name) {
	auto module = std::make_unique<Module>(context_, name);
	Module* m = module.get();
	modules_.insert({ m->module(),std::move(module) });
	return m;
}


std::unique_ptr<Module> Context::GetModule(JSModuleDef* m) {
	auto find = modules_.find(m);
	if (find != modules_.end()) {
		std::unique_ptr<Module> module = std::move(find->second);
		modules_.erase(find);
		return module;
	}
	return nullptr;
}


//�����ֽ���
bool Context::LoadByteCode(const uint8_t* buf, size_t buf_len) {

	Value obj(context_,JS_ReadObject(context_, buf, buf_len, JS_READ_OBJ_BYTECODE));

	if (obj.IsException()) {
		return false;
	}

	if (obj.tag() == JS_TAG_MODULE) {
		if (JS_ResolveModule(context_, obj) < 0) {
			return false;
		}
		js_module_set_import_meta(context_, obj, false, true);
	}

	Value val(context_,JS_EvalFunction(context_, obj));
	if (val.IsException()) {
		DumpError();
		return false;
	}
	return true;
}

Value Context::NewUint8ArrayBuffer(const uint8_t* buf, size_t len) {
	Value abuf = NewArrayBuffer(buf, len);

	if (abuf.IsException())
		return abuf;

	Value global = Global();
	Value u8array_ctor = global.GetProperty("Uint8Array");
	return u8array_ctor.Call(abuf);
}


Value Context::ThrowSyntaxError(const char* fmt, ...) {
	JSValue val;
	va_list ap;

	va_start(ap, fmt);
	val = JS_ThrowError(context_, JS_SYNTAX_ERROR, fmt, ap);
	va_end(ap);
	return Value(context_,std::move(val));
}

Value Context::ThrowTypeError(const char* fmt, ...) {
	JSValue val;
	va_list ap;

	va_start(ap, fmt);
	val = JS_ThrowError(context_, JS_TYPE_ERROR, fmt, ap);
	va_end(ap);
	return Value(context_, std::move(val));
}

Value Context::ThrowReferenceError(const char* fmt, ...) {
	JSValue val;
	va_list ap;

	va_start(ap, fmt);
	val = JS_ThrowError(context_, JS_REFERENCE_ERROR, fmt, ap);
	va_end(ap);
	return Value(context_, std::move(val));
}

Value Context::ThrowRangeError(const char* fmt, ...) {
	JSValue val;
	va_list ap;

	va_start(ap, fmt);
	val = JS_ThrowError(context_, JS_RANGE_ERROR, fmt, ap);
	va_end(ap);
	return Value(context_, std::move(val));
}

Value Context::ThrowInternalError(const char* fmt, ...) {
	JSValue val;
	va_list ap;

	va_start(ap, fmt);
	val = JS_ThrowError(context_, JS_INTERNAL_ERROR, fmt, ap);
	va_end(ap);
	return Value(context_, std::move(val));
}

Value Context::ThrowOutOfMemory() {
	return Value(context_, JS_ThrowOutOfMemory(context_));
}

void Context::DumpError() {
	JSValue exception_val;
	exception_val = JS_GetException(context_);
	std::string error = dump_error(context_, exception_val);
	JS_FreeValue(context_, exception_val);

	if (log_func_) {
		log_func_(error);
	} else {
		printf("%s", error.c_str());
	}
}


uint8_t* WeakValue::ToBuffer(size_t* psize) const {
	size_t aoffset, asize, size;
	JSValue abuf = JS_GetTypedArrayBuffer(context_, value_, &aoffset, &asize, NULL);
	if (JS_IsException(abuf)) {
		Context::get(context_)->DumpError();
		return nullptr;
	}

	uint8_t* buf = JS_GetArrayBuffer(context_, &size, abuf);
	if (buf) {
		buf += aoffset;
		size = asize;
	} else {
		size = 0;
	}
	JS_FreeValue(context_, abuf);

	*psize = size;
	return buf;
}


Value WeakValue::GetProperty(const char* prop) const {
	return Value(context_, JS_GetPropertyStr(context_, value_, prop));
}

Value WeakValue::GetProperty(uint32_t idx) const {
	return Value(context_, JS_GetPropertyUint32(context_, value_, idx));
}

size_t WeakValue::length() const {
	if (!IsObject()) {
		return 0;
	}

	Value val = GetProperty("length");
	return val.ToUint32();
}

std::map<std::string, Value> WeakValue::GetProperties(int flags) {
	std::map<std::string, Value> properties;
	if (!IsObject() || !context_) {
		return properties;
	}

	uint32_t len;
	JSPropertyEnum* property;
	if (JS_GetOwnPropertyNames(context_, &property, &len, value_, flags) == 0) {
		for (uint32_t i = 0; i < len; ++i) {
			const char* name = JS_AtomToCString(context_, property[i].atom);
			properties[name] = Value(context_, JS_GetProperty(context_, value_, property[i].atom));
			JS_FreeCString(context_, name);
		}
	}
	return properties;
}

bool WeakValue::SetPrototype(const Value& value) {
	return JS_SetPrototype(context_, value_, value.value_) == 0;
}


Value WeakValue::GetPrototype() {
	return Value(context_, JS_GetPrototype(context_, value_));
}


Value WeakValue::Call() const {
	return Value(context_, JS_Call(context_, value_, JS_UNDEFINED, 0, nullptr));
}


Value WeakValue::Invoke(const char* func_name) const {
	auto atom = JS_NewAtom(context_, func_name);
	JSValue rslt = JS_Invoke(context_, value_, atom, 0, nullptr);
	JS_FreeAtom(context_, atom);
	return Value(context_, std::move(rslt));
}


Value WeakValue::ExcuteBytecode() {
	return Value(context_,
		JS_EvalFunction(context_, JS_DupValue(context_, value_)));
}

}//namespace


