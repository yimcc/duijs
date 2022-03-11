﻿#pragma once
#include "include/quickjs.h"
#include "include/quickjs-libc.h"
#include <utility>
#include <memory>
#include <map>
#include <unordered_map>
#include <string>
#include <array>
#include <functional>
#include <assert.h>

namespace qjs {

#define QJS_DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&) = delete;          \
  TypeName& operator=(const TypeName&) = delete


class Value;
class Context;
class ArgList;
template<class T> class Class;
template<class T> class WeakClass;
template<class T> class RefClass;


class Runtime {
public:
	Runtime()
		:runtime_(JS_NewRuntime())
	{
		//JS_SetModuleLoaderFunc(runtime_, NULL, js_module_loader, NULL);
		js_std_init_handlers(runtime_);
		JS_SetHostPromiseRejectionTracker(runtime_, PromiseRejectionTracker, NULL);
	}

	~Runtime() {
		js_std_free_handlers(runtime_);
		JS_FreeRuntime(runtime_);
	}

	void SetRuntimeInfo(const char* info) {
		JS_SetRuntimeInfo(runtime_, info);
	}

	void SetMemoryLimit(size_t limit) {
		JS_SetMemoryLimit(runtime_, limit);
	}

	void SetGCThreshold(size_t gc_threshold) {
		JS_SetGCThreshold(runtime_, gc_threshold);
	}

	void SetMaxStackSize(size_t stack_size) {
		JS_SetMaxStackSize(runtime_, stack_size);
	}

	JSRuntime* runtime() {
		return runtime_;
	}
private:
	static void PromiseRejectionTracker(JSContext* ctx, JSValueConst promise,
		JSValueConst reason,
		JS_BOOL is_handled, void* opaque);
	friend class Context;
	QJS_DISALLOW_COPY_AND_ASSIGN(Runtime);
	JSRuntime* runtime_;
};


//用于导出模块
class Module {
public:
	Module(JSContext* ctx, const char* name);
	~Module();

	//导出到模块
	bool Export(const char* name, const Value& value);
	bool Export(const char* name, JSValue value);

	bool ExportString(const char* name, const char* value) {
		return Export(name, JS_NewString(context_, value));
	}

	bool ExportInt32(const char* name, int32_t value) {
		return Export(name, JS_NewInt32(context_, value));
	}

	bool ExportUint32(const char* name, uint32_t value) {
		return Export(name, JS_NewUint32(context_, value));
	}

	bool ExportInt64(const char* name, int64_t value) {
		return Export(name, JS_NewInt64(context_, value));
	}


	bool ExportUint64(const char* name, uint64_t value) {
		return Export(name, JS_NewBigUint64(context_, value));
	}

	bool ExportFloat32(const char* name, float value) {
		return Export(name, JS_NewFloat64(context_, value));
	}

	bool ExportFloat64(const char* name, double value) {
		return Export(name, JS_NewFloat64(context_, value));
	}

	bool ExportCFunc(const char* name, JSCFunction* func);

	template<Value func(Context& context, ArgList& args)>
	bool ExportFunc(const char* name);


	Value GetImportMeta();

	inline JSModuleDef* module() {
		return module_;
	}

	inline Context* context();

	template<class T>
	Class<T> ExportClass(const char* name);


	template<class T>
	WeakClass<T> ExportWeakClass(const char* name);

	template<class T>
	RefClass<T> ExportRefClass(const char* name);
private:
	QJS_DISALLOW_COPY_AND_ASSIGN(Module);
	static int OnInit(JSContext* ctx, JSModuleDef* m);
	JSContext* context_;
	JSModuleDef* module_;
	std::map<std::string, Value> exports_;
};


class Context {
public:
	Context(Runtime* runtime);
	Context(Runtime* runtime, int argc, char** argv);
	Context(JSContext* context);
	~Context();

	JSRuntime* runtime() {
		return JS_GetRuntime(context_);
	}

	JSContext* context() {
		return context_;
	}

	static Context* get(JSContext* ctx) {
		return reinterpret_cast<Context*>(
			JS_GetContextOpaque(ctx));
	}

	void SetUserData(void* ud) { user_data_ = ud; }
	void* user_data() { return user_data_; }
	void SetLogFunc(std::function<void(const std::string& msg)> func) {
		log_func_ = func;
	}

	Value ParseJson(const char* buf, size_t buf_len,
		const char* filename);

	//new
	Value NewNull();
	Value NewBool(bool v);
	Value NewInt32(int32_t v);
	Value NewInt64(int64_t v);
	Value NewUint32(uint32_t v);
	Value NewBigInt64(int32_t v);
	Value NewBigUint64(int64_t v);
	Value NewFloat64(double v);

	Value NewString(const char* str);
	Value NewString(const char* str, size_t len);


	Value NewObject();
	Value NewArray();

	//创建class的对象
	Value NewClassObject(JSClassID class_id);

	Value NewArrayBuffer(const uint8_t* buf, size_t len);

	Value NewUint8ArrayBuffer(const uint8_t* buf, size_t len);

	/* Note: at least 'length' arguments will be readable in 'argv' */
	Value NewCFunction(JSCFunction* func, const char* name, size_t length);

	template<Value func(Context& context, ArgList& args)>
	Value NewFunction(const char* name);


	Value Global();

	Value Excute(const char* input, size_t input_len,
		const char* filename);


	Value Excute(const char* input, size_t input_len,
		const char* filename, int flags);

	Value Compile(const char* input, size_t input_len,
		const char* filename);

	//返回false，调用DumpError()查看错误
	bool LoadByteCode(const uint8_t* buf, size_t buf_len);

	void Loop() {
		js_std_loop(context_);
	}

	void ExecuteJobs();

	void DumpError();

	//创建模块
	Module* NewModule(const char* name);

	//抛出js异常
	Value ThrowSyntaxError(const char* fmt, ...);
	Value ThrowTypeError(const char* fmt, ...);
	Value ThrowReferenceError(const char* fmt, ...);
	Value ThrowRangeError(const char* fmt, ...);
	Value ThrowInternalError(const char* fmt, ...);
	Value ThrowOutOfMemory();

	void RunGC() {
		JS_RunGC(JS_GetRuntime(context_));
	}

	void Log(const std::string& msg) {
		if (log_func_)
			log_func_(msg);
	}

	void AddClassId(JSClassID classid, JSClassID parent_classid);
	JSClassID GetParentClassId(JSClassID classid);

private:
	QJS_DISALLOW_COPY_AND_ASSIGN(Context);

	friend class Value;
	friend class Module;

	void Init(int argc, char** argv);
	std::unique_ptr<Module> GetModule(JSModuleDef* m);

	void* user_data_{ nullptr };
	JSContext* context_;
	std::map<JSModuleDef*, std::unique_ptr<Module>> modules_;

	std::function<void(const std::string& msg)> log_func_;

	//存储classid，及其父classid
	std::unordered_map<JSClassID, JSClassID> class_ids_;
};


class String {
public:
	String(JSContext* ctx, const char* str, size_t len)
		:context_(ctx), str_(str), len_(len)
	{
		if (!str_) {
			str_ = "";
			len_ = 0;
		}
	}

	String(const char* str, size_t len)
		:context_(nullptr), str_(str), len_(len)
	{
		if (!str_) {
			str_ = "";
			len_ = 0;
		}
	}

	String(String&& view) noexcept {
		context_ = view.context_;
		str_ = view.str_;
		len_ = view.len_;
		view.context_ = nullptr;
		view.str_ = nullptr;
		view.len_ = 0;
	}

	~String() {
		if (context_)
			JS_FreeCString(context_, str_);
	}

	const char* str() const {
		if (str_)
			return str_;
		else
			return "";
	}

	size_t len() const {
		return len_;
	}
private:
	QJS_DISALLOW_COPY_AND_ASSIGN(String);

	JSContext* context_;
	const char* str_;
	size_t len_;
};

class Value;

//弱引用值
class WeakValue{
public:
	WeakValue()
		:context_(nullptr), value_(JS_UNDEFINED)
	{
	}

	WeakValue(JSContext* ctx)
		:context_(ctx), value_(JS_UNDEFINED)
	{
	}

	WeakValue(JSContext* ctx, const JSValueConst& value)
		:context_(ctx), value_(value)
	{
	}

	WeakValue(const WeakValue& value)
		:context_(value.context_),
		value_(value.value_)
	{
	}

	WeakValue& operator=(const WeakValue& value) {
		context_ = value.context_;
		value_ = value.value_;
		return *this;
	}

	operator JSValue() const {
		return value_;
	}

	int tag() {
		return JS_VALUE_GET_TAG(value_);
	}


	Context* context() const {
		return reinterpret_cast<Context*>(
			JS_GetContextOpaque(context_));
	}

	bool IsNumber() const {
		return JS_IsNumber(value_);
	}

	bool IsBigInt() const {
		return JS_IsBigInt(context_, value_);
	}

	bool IsBigFloat() const {
		return JS_IsBigFloat(value_);
	}

	bool IsBigDecimal() const {
		return JS_IsBigDecimal(value_);
	}

	bool IsBool() const {
		return JS_IsBool(value_);
	}

	bool IsNull() const {
		return JS_IsNull(value_);
	}

	bool IsUndefined() const {
		return JS_IsUndefined(value_);
	}

	bool IsException() const {
		return JS_IsException(value_);
	}

	bool IsUninitialized() const {
		return JS_IsUninitialized(value_);
	}

	bool IsString() const {
		return JS_IsString(value_);
	}

	bool IsSymbol() const {
		return JS_IsSymbol(value_);
	}

	bool IsObject() const {
		return JS_IsObject(value_);
	}

	bool IsArray() const {
		return JS_IsArray(context_, value_);
	}

	bool IsFunction()const {
		return JS_IsFunction(context_, value_);
	}

	bool ToBool() const {
		return JS_ToBool(context_, value_) == 1;
	}

	bool IsError() const {
		return JS_IsError(context_, value_) != 0;
	}


	int32_t ToInt32() const {
		int32_t value = 0;
		JS_ToInt32(context_, &value, value_);
		return value;
	}

	uint32_t ToUint32() const {
		uint32_t value = 0;
		JS_ToUint32(context_, &value, value_);
		return value;
	}


	int64_t ToInt64() const {
		int64_t value = 0;
		JS_ToInt64(context_, &value, value_);
		return value;
	}

	//uint64_t ToIndex() const {
	//	uint64_t value = 0;
	//	JS_ToIndex(context_, &value, value_);
	//	return value;
	//}


	float ToFloat() const {
		double value = 0;
		JS_ToFloat64(context_, &value, value_);
		return (float)value;
	}

	double ToFloat64() const {
		double value = 0;
		JS_ToFloat64(context_, &value, value_);
		return value;
	}

	int64_t ToBigInt64() const {
		int64_t value = 0;
		JS_ToBigInt64(context_, &value, value_);
		return value;
	}

	int64_t ToInt64Ext() const {
		int64_t value = 0;
		JS_ToInt64Ext(context_, &value, value_);
		return value;
	}

	String ToString() const {
		if (!IsString()) {
			JSValue value = JS_ToString(context_, value_);
			size_t len;
			const char* str = JS_ToCStringLen(context_, &len, value);
			JS_FreeValue(context_, value);
			return String(context_, str, len);
		}

		size_t len;
		const char* str = JS_ToCStringLen(context_, &len, value_);
		return String(context_, str, len);
	}

	uint8_t* ToBuffer(size_t* psize) const;

	std::string ToStdString() const {
		String ss = ToString();
		return std::string(ss.str(), ss.len());
	}

	void SetOpaque(void* opaque) {
		JS_SetOpaque(value_, opaque);
	}

	void* GetOpaque(JSClassID class_id) const {
		return JS_GetOpaque2(context_, value_, class_id);
	}


	//proto接口
	void SetPropertyFunctionList(const JSCFunctionListEntry* tab,
		int len) {
		JS_SetPropertyFunctionList(context_, value_, tab, len);
	}

	void SetConstructor(JSCFunction* func, const char* name) {
		JSValue func_obj = JS_NewCFunction2(context_, func, name, 1, JS_CFUNC_constructor, 0);
		JS_SetConstructor(context_, func_obj, value_);
	}

	//gc标记
	void Mark(JS_MarkFunc* mark_func) {
		JS_MarkValue(JS_GetRuntime(context_), value_, mark_func);
	}

	//获取属性
	Value GetProperty(const char* prop) const;
	Value GetProperty(uint32_t idx) const;

	//获取数组的长度
	size_t length() const;

	std::map<std::string, Value> 
		GetProperties(int flags = JS_GPN_STRING_MASK | JS_GPN_SYMBOL_MASK);

	bool HasProperty(const char* prop) const {
		if (!context_) {
			return false;
		}
		auto atom = JS_NewAtom(context_, prop);
		bool has = JS_HasProperty(context_, value_, atom);
		JS_FreeAtom(context_, atom);
		return has;
	}

	bool HasProperty(uint32_t idx) const {
		if (!context_) {
			return false;
		}
		auto atom = JS_NewAtomUInt32(context_, idx);
		bool has = JS_HasProperty(context_, value_, atom);
		JS_FreeAtom(context_, atom);
		return has;
	}

	bool DeleteProperty(const char* prop) {
		if (!context_) {
			return false;
		}
		auto atom = JS_NewAtom(context_, prop);
		int rslt = JS_DeleteProperty(context_, value_, atom, 0);
		JS_FreeAtom(context_, atom);
		return rslt == 1;
	}

	bool DeleteProperty(uint32_t idx) {
		if (!context_) {
			return false;
		}
		auto atom = JS_NewAtomUInt32(context_, idx);
		int rslt = JS_DeleteProperty(context_, value_, atom, 0);
		JS_FreeAtom(context_, atom);
		return rslt == 1;
	}

	bool SetProperty(const char* key, const WeakValue& value) {
		if (!context_) {
			return false;
		}
		return JS_SetPropertyStr(context_, value_, key,
			JS_DupValue(context_, value.value_)) == 0;
	}


	bool SetPropertyString(uint32_t key, const char* value) {
		if (!context_) {
			return false;
		}
		return JS_SetPropertyUint32(context_, value_, key, JS_NewString(context_, value)) == 0;
	}

	bool SetPropertyString(uint32_t key, const char* value, size_t len) {
		if (!context_) {
			return false;
		}
		return JS_SetPropertyUint32(context_, value_, key, JS_NewStringLen(context_, value, len)) == 0;
	}

	bool SetPropertyInt32(uint32_t key, int32_t value) {
		if (!context_) {
			return false;
		}
		return JS_SetPropertyUint32(context_, value_, key, JS_NewInt32(context_, value)) == 0;
	}

	bool SetPropertyInt64(uint32_t key, int64_t value) {
		if (!context_) {
			return false;
		}
		return JS_SetPropertyUint32(context_, value_, key, JS_NewInt64(context_, value)) == 0;
	}

	bool SetPropertyFloat32(uint32_t key, float value) {
		if (!context_) {
			return false;
		}
		return JS_SetPropertyUint32(context_, value_, key, JS_NewFloat64(context_, value)) == 0;
	}

	bool SetPropertyFloat64(uint32_t key, double value) {
		if (!context_) {
			return false;
		}
		return JS_SetPropertyUint32(context_, value_, key, JS_NewFloat64(context_, value)) == 0;
	}

	bool SetPropertyString(const char* key, const char* value) {
		if (!context_) {
			return false;
		}
		return JS_SetPropertyStr(context_, value_, key, JS_NewString(context_, value)) == 0;
	}

	bool SetPropertyString(const char* key, const char* value, size_t len) {
		if (!context_) {
			return false;
		}
		return JS_SetPropertyStr(context_, value_, key, JS_NewStringLen(context_, value, len)) == 0;
	}

	bool SetPropertyInt32(const char* key, int32_t value) {
		if (!context_) {
			return false;
		}
		return JS_SetPropertyStr(context_, value_, key, JS_NewInt32(context_, value)) == 0;
	}

	bool SetPropertyInt64(const char* key, int64_t value) {
		if (!context_) {
			return false;
		}
		return JS_SetPropertyStr(context_, value_, key, JS_NewInt64(context_, value)) == 0;
	}

	bool SetPropertyFloat32(const char* key, float value) {
		if (!context_) {
			return false;
		}
		return JS_SetPropertyStr(context_, value_, key, JS_NewFloat64(context_, value)) == 0;
	}

	bool SetPropertyFloat64(const char* key, double value) {
		if (!context_) {
			return false;
		}
		return JS_SetPropertyStr(context_, value_, key, JS_NewFloat64(context_, value)) == 0;
	}

	bool SetPrototype(const Value& value);
	Value GetPrototype();

	uint8_t* GetArrayBuffer(size_t* psize) {
		return JS_GetArrayBuffer(context_, psize, value_);
	}

	//执行普通函数，this_obj为JS_UNDEFINED
	Value Call() const;

	template<typename ...Args>
	Value Call(Args&&... args) const;

	//执行对象的函数
	Value Invoke(const char* func_name) const;

	template<typename ...Args>
	Value Invoke(const char* func_name, Args&&... args) const;

	Value ExcuteBytecode();
protected:
	friend class ConstValue;
	JSContext* context_;
	JSValue value_;
};

//强引用值
class Value:public WeakValue{
public:
	Value()
		:WeakValue()
	{
	}

	Value(JSContext* ctx)
		:WeakValue(ctx)
	{
	}

	Value(JSContext* ctx, JSValue&& value)
		:WeakValue(ctx,value)
	{
		value = JS_UNDEFINED;
	}

	Value(JSContext* ctx, const JSValueConst& value)
		:WeakValue(ctx,JS_DupValue(ctx, value))
	{
	}

	Value(const Value& value)
		:WeakValue(value.context_, JS_DupValue(context_, value.value_))
	{
	}

	Value(Value&& value) noexcept 
		:WeakValue(value.context_, value.value_)
	{
		value.value_ = JS_UNDEFINED;
	}

	~Value() {
		if (context_) {
			JS_FreeValue(context_, value_);
		}
	}

	Value& operator=(const Value& value) {
		JSValue temp = JS_DupValue(context_, value.value_);
		JS_FreeValue(context_, value_);
		context_ = value.context_;
		value_ = temp;
		return *this;
	}

	JSValue CopyValue() const {
		return JS_DupValue(context_, value_);
	}

	JSValue Release() {
		JSValue value = value_;
		value_ = JS_UNDEFINED;
		return value;
	}


	bool SetProperty(const char* key, const Value& value) {
		return JS_SetPropertyStr(context_, value_, key,
			JS_DupValue(context_, value.value_)) == 0;
	}


	bool SetProperty(const char* key, Value&& value) {
		JSValue temp = value.value_;
		value.value_ = JS_UNDEFINED;
		return JS_SetPropertyStr(context_, value_, key, temp) == 0;
	}

	bool SetProperty(uint32_t key, const Value& value) {
		return JS_SetPropertyUint32(context_, value_, key,
			JS_DupValue(context_, value.value_)) == 0;
	}

	bool SetProperty(uint32_t key, Value&& value) {
		JSValue temp = value.value_;
		value.value_ = JS_UNDEFINED;
		return JS_SetPropertyUint32(context_, value_, key, temp) == 0;
	}


};



template<typename ...Args>
Value WeakValue::Call(Args&&... args) const {
	JSValue params[] = { std::forward<Args>(args)... };
	return Value(context_, JS_Call(context_, value_, JS_UNDEFINED, sizeof...(args), params));
}

template<typename ...Args>
Value WeakValue::Invoke(const char* func_name, Args&&... args) const {
	JSValue params[] = { std::forward<Args>(args)... };
	auto atom = JS_NewAtom(context_, func_name);
	JSValue rslt = JS_Invoke(context_, value_, atom, sizeof...(args), params);
	JS_FreeAtom(context_, atom);
	return Value(context_, std::move(rslt));
}



extern const Value undefined_value;
extern const Value null_value;
extern const Value true_value;
extern const Value false_value;
extern const Value exception_value;
extern const Value uninit_value;


//参数列表
class ArgList {
public:
	enum {
		MaxArgCount = 16
	};

	ArgList()
		:size_(0)
	{
	}

	ArgList(JSContext* context, int argc, JSValueConst* argv)
		:size_(argc)
	{
		assert(size_ < MaxArgCount);
		for (size_t i = 0; i < size_; ++i) {
			args_[i] = Value(context, argv[i]);
		}
	}

	template<class ...Args>
	ArgList(Args ...args)
		:size_(sizeof...(args))
	{
		assert(size_ < MaxArgCount);
		args_ = { std::forward<Args>(args)... };
	}

	inline size_t size() const {
		return size_;
	}

	Value operator[](size_t idx) const {
		if (idx >= size_) {
			return undefined_value;
		}
		return args_[idx];
	}

	Value arg(size_t idx) const {
		if (idx >= size_) {
			return undefined_value;
		}
		return args_[idx];
	}
private:
	QJS_DISALLOW_COPY_AND_ASSIGN(ArgList);
	size_t size_;
	std::array<Value, MaxArgCount> args_;
};



template<class T>
bool GetThis(Context* ctx,JSValueConst this_val, T** pThis, JSClassID cid) {
	if (!JS_IsObject(this_val)) {
		return false;
	}

	JSClassID id = JS_GetClassID(this_val);
	if (cid) {
		JSClassID pid = id;
		while (pid) {
			if (pid == cid) {
				//如果id或者其父class id与class_id_相同则允许转换
				*pThis = reinterpret_cast<T*>(JS_GetOpaque(this_val, id));
				return *pThis != nullptr;
			}
			pid = ctx->GetParentClassId(pid);
		}
		return false;
	}
	*pThis = reinterpret_cast<T*>(JS_GetOpaque(this_val, id));
	return *pThis != nullptr;
}

class ClassBase {
public:
	ClassBase(JSContext* context, Module* module, const char* name)
		:context_(context),
		prototype_(context, JS_NewObject(context)),
		module_(module), class_inited_(false), class_name_(name)
	{
	}

	~ClassBase() {
		assert(class_inited_);
	}


	//添加属性等
	void AddValue(const char* name, const Value& value) {
		JS_DefinePropertyValueStr(context_, prototype_, name, value.CopyValue(), 0);
	}

	void AddString(const char* name, const char* value) {
		JS_DefinePropertyValueStr(context_, prototype_, name, JS_NewString(context_, value), 0);
	}

	void AddString(const char* name, const char* value, size_t len) {
		JS_DefinePropertyValueStr(context_, prototype_, name, JS_NewStringLen(context_, value, len), 0);
	}

	void AddInt32(const char* name, int32_t value) {
		JS_DefinePropertyValueStr(context_, prototype_, name, JS_NewInt32(context_, value), 0);
	}

	void AddInt64(const char* name, int32_t value) {
		JS_DefinePropertyValueStr(context_, prototype_, name, JS_NewInt64(context_, value), 0);
	}

	void AddFloat(const char* name, float value) {
		JS_DefinePropertyValueStr(context_, prototype_, name, JS_NewFloat64(context_, value), 0);
	}

	void AddFloat64(const char* name, double value) {
		JS_DefinePropertyValueStr(context_, prototype_, name, JS_NewFloat64(context_, value), 0);
	}

protected:
	JSContext* context_;
	Value prototype_;
	Module* module_;
	bool class_inited_;
	const char* class_name_;
};


template<class T>
bool GetThis(JSValueConst this_val, T** pThis) {
	if (!JS_IsObject(this_val)) {
		return false;
	}
	JSClassID id = JS_GetClassID(this_val);
	*pThis = reinterpret_cast<T*>(JS_GetOpaque(this_val, id));
	return *pThis != nullptr;
}


template<class T>
void SetThis(JSValueConst this_val, T* pThis) {
	JS_SetOpaque(this_val, pThis);
}


template<class T>
class Class : public ClassBase {
public:
	Class(JSContext* context, Module* module, const char* name)
		:ClassBase(context, module, name)
	{
		if (class_id_ == 0) {
			JS_NewClassID(&class_id_);
		}
	}

	//转为c
	static T* ToC(const Value& v) {
		auto context = v.context();
		if (!context) {
			return nullptr;
		}
		T* pThis = nullptr;
		return GetThis(context,v, &pThis, class_id_) ? pThis : nullptr;
	}

	//创建新的js对象，注意js对象释放时会调用dtor释放ptr
	static Value ToJs(Context& context, T* ptr) {
		if (!ptr)
			return null_value;

		Value obj = context.NewClassObject(class_id_);
		SetThis(obj, ptr);
		return obj;
	}

	static Value ToJsById(Context& context, T* ptr, JSClassID cid) {
		if (!ptr)
			return null_value;
		//TODO:检测cid为class_id_的子类
		Value obj = context.NewClassObject(cid);
		SetThis(obj, ptr);
		return obj;
	}


	void Init(JSClassFinalizer* finalizer, JSClassID parent_id = 0) {
		assert(!class_inited_);
		JSClassDef class_def = {
			class_name_,finalizer,nullptr,nullptr,nullptr
		};
		NewClass(&class_def, parent_id);
	}

	void Init(JSClassFinalizer* finalizer, JSClassGCMark* gc_mark, JSClassID parent_id = 0) {
		assert(!class_inited_);
		JSClassDef class_def = {
			class_name_,finalizer,gc_mark,nullptr,nullptr
		};
		NewClass(&class_def, parent_id);
	}

	template<void dtor(T*)>
	void Init(JSClassID parent_id = 0) {
		assert(!class_inited_);

		JSClassDef class_def = {
			class_name_,[](JSRuntime* rt, JSValue val) {
				T* s = nullptr;
				if (GetThis(val, &s)) {
					dtor(s);
				}
			},nullptr,nullptr,nullptr
		};

		NewClass(&class_def, parent_id);
	}


	template<void dtor(T*), void mark(T*, JS_MarkFunc* mark_func)>
	void Init2(JSClassID parent_id = 0) {
		assert(!class_inited_);

		JSClassDef class_def = {
			class_name_,
			[](JSRuntime* rt, JSValue val) {
				T* s = nullptr;
				if (GetThis(val,&s)) {
					dtor(s);
				}
			},
			[](JSRuntime* rt, JSValueConst val, JS_MarkFunc* mark_func) {
				T* s = nullptr;
				if (GetThis(val,&s)) {
					mark(s, mark_func);
				}
			},nullptr,nullptr
		};

		NewClass(&class_def, parent_id);
	}


	template<T* ctor(Context& context, ArgList& args)>
	void AddCtor() {
		JSValue constructor = JS_NewCFunction2(context_, [](JSContext* ctx,
			JSValueConst new_target,
			int argc, JSValueConst* argv) {

				Value proto(ctx, JS_GetPropertyStr(ctx, new_target, "prototype"));
				if (proto.IsException()) {
					return JS_EXCEPTION;
				}
				Context* context = Context::get(ctx);
				ArgList arg_list(ctx, argc, argv);
				T* pThis = ctor(*context, arg_list);
				if (!pThis) {
					return JS_ThrowInternalError(ctx, "ctor error");
				}

				Value obj(ctx, JS_NewObjectProtoClass(ctx, proto, class_id_));

				if (obj.IsException()) {
					return JS_EXCEPTION;
				}
				SetThis(obj, pThis);
				return obj.Release();
			}, class_name_, 0, JS_CFUNC_constructor, 0);
		JS_SetConstructor(context_, constructor, prototype_);
		module_->Export(class_name_, constructor);
	}

	template<T* ctor(Context& context, Value& this_obj, ArgList& args)>
	void AddCtor2() {
		JSValue constructor = JS_NewCFunction2(context_, [](JSContext* ctx,
			JSValueConst new_target,
			int argc, JSValueConst* argv) {

				Value proto(ctx, JS_GetPropertyStr(ctx, new_target, "prototype"));
				if (proto.IsException()) {
					return JS_EXCEPTION;
				}
				Context* context = Context::get(ctx);
				ArgList arg_list(ctx, argc, argv);

				Value obj(ctx, JS_NewObjectProtoClass(ctx, proto, class_id_));

				if (obj.IsException()) {
					return JS_EXCEPTION;
				}

				T* pThis = ctor(*context, obj, arg_list);
				if (!pThis) {
					return JS_ThrowInternalError(ctx, "ctor error");
				}

				SetThis(obj, pThis);
				return obj.Release();
			}, class_name_, 0, JS_CFUNC_constructor, 0);
		JS_SetConstructor(context_, constructor, prototype_);
		module_->Export(class_name_, constructor);
	}

	template<Value func(T* pThis, Context& context, ArgList& args)>
	void AddFunc(const char* name) {
		JS_DefinePropertyValueStr(context_, prototype_, name,
			JS_NewCFunction(context_,
				[](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) {
					T* pThis = nullptr;
					if (!GetThis(this_val, &pThis)) {
						return JS_ThrowTypeError(ctx, "no this pointer exist");
					}
					Context* context = Context::get(ctx);
					ArgList arg_list(ctx, argc, argv);
					return func(pThis, *context, arg_list).Release();
				}, name, 0), 0);
	}

	template<JSCFunction func>
	void AddCFunc(const char* name) {
		JS_DefinePropertyValueStr(context_, prototype_, name, JS_NewCFunction(context_, func, name, 0));
	}

	template<Value get(T* pThis, Context& context), void set(T* pThis, Value arg)>
	void AddGetSet(const char* name) {
		JSCFunctionType get_func;
		get_func.getter = [](JSContext* ctx, JSValueConst this_val) {
			T* pThis = nullptr;
			if (!GetThis(this_val, &pThis)) {
				JS_ThrowTypeError(ctx, "no this pointer exist");
				return JS_EXCEPTION;
			}

			Context* context = Context::get(ctx);
			return get(pThis, *context).Release();
		};

		JSValue get_func_value = JS_NewCFunction2(context_, get_func.generic, name, 0, JS_CFUNC_getter, 0);


		JSCFunctionType set_func;
		set_func.setter = [](JSContext* ctx, JSValueConst this_val, JSValueConst val) {
			T* pThis = nullptr;
			if (!GetThis(this_val, &pThis)) {
				JS_ThrowTypeError(ctx, "no this pointer exist");
				return JS_EXCEPTION;
			}
			Value arg(ctx, val);
			set(pThis, arg);
			return JS_UNDEFINED;
		};

		JSValue set_func_value = JS_NewCFunction2(context_, set_func.generic, name, 1, JS_CFUNC_setter, 0);
		auto atom = JS_NewAtom(context_, name);
		JS_DefinePropertyGetSet(context_, prototype_, atom, get_func_value, set_func_value, 0);
		JS_FreeAtom(context_, atom);
	}


	template<Value get(T* pThis, Context& context)>
	void AddGet(const char* name) {
		JSCFunctionType get_func;
		get_func.getter = [](JSContext* ctx, JSValueConst this_val) {
			T* pThis = nullptr;
			if (!GetThis(this_val, &pThis)) {
				JS_ThrowTypeError(ctx, "no this pointer exist");
				return JS_EXCEPTION;
			}

			Context* context = Context::get(ctx);
			return get(pThis, *context).Release();
		};

		JSValue get_func_value = JS_NewCFunction2(context_, get_func.generic, name, 0, JS_CFUNC_getter, 0);

		auto atom = JS_NewAtom(context_, name);

		JS_DefineProperty(context_, prototype_, atom, JS_UNDEFINED, get_func_value, JS_UNDEFINED,
			JS_PROP_HAS_GET | JS_PROP_HAS_CONFIGURABLE | JS_PROP_HAS_ENUMERABLE);
		JS_FreeValue(context_, get_func_value);
		JS_FreeAtom(context_, atom);
	}

	template<void set(T* pThis, Value arg)>
	void AddSet(const char* name) {
		JSCFunctionType set_func;
		set_func.setter = [](JSContext* ctx, JSValueConst this_val, JSValueConst val) {
			T* pThis = nullptr;
			if (!GetThis(this_val, &pThis)) {
				JS_ThrowTypeError(ctx, "no this pointer exist");
				return JS_EXCEPTION;
			}
			Value arg(ctx, val);
			set(pThis, arg);
			return JS_UNDEFINED;
		};

		JSValue set_func_value = JS_NewCFunction2(context_, set_func.generic, name, 1, JS_CFUNC_setter, 0);
		auto atom = JS_NewAtom(context_, name);
		JS_DefineProperty(context_, prototype_, atom, JS_UNDEFINED, JS_UNDEFINED, set_func_value,
			JS_PROP_HAS_SET | JS_PROP_HAS_CONFIGURABLE | JS_PROP_HAS_ENUMERABLE);
		JS_FreeValue(context_, set_func_value);
		JS_FreeAtom(context_, atom);
	}

	template<Value itr(T* pThis, Context& context, ArgList& arglist, bool& finish)>
	void AddIterator(const char* name) {
		JSCFunctionType itr_func;
		itr_func.iterator_next = [](JSContext* ctx, JSValueConst this_val,
			int argc, JSValueConst* argv, int* pdone, int magic) {
				T* pThis = nullptr;
				if (!GetThis(this_val, &pThis)) {
					JS_ThrowTypeError(ctx, "no this pointer exist");
					return JS_EXCEPTION;
				}

				Context* context = Context::get(ctx);
				ArgList arg_list(ctx, argc, argv);
				bool finish = false;
				Value result = itr(pThis, *context, arg_list, finish);
				*pdone = finish;
				return result.Release();
		};

		JSValue itr_func_value = JS_NewCFunction2(context_, itr_func.generic, name, 0, JS_CFUNC_iterator_next, 0);
		auto atom = JS_NewAtom(context_, name);
		JS_DefinePropertyValue(context_, prototype_, atom, itr_func_value, 0);
		JS_FreeAtom(context_, atom);
	}

	static JSClassID class_id() {
		return class_id_;
	}
private:
	void NewClass(JSClassDef* class_def, JSClassID parent_id = 0) {
		assert(!class_inited_);
		JS_NewClass(JS_GetRuntime(context_), class_id_, class_def);
		JS_SetClassProto(context_, class_id_, prototype_.CopyValue());
		class_inited_ = true;

		if (parent_id) {
			Context::get(context_)->AddClassId(class_id_, parent_id);

			JSValue parent = JS_GetClassProto(context_, parent_id);
			if (JS_IsObject(parent)) {
				JS_SetPrototype(context_, prototype_, parent);
			}
			else {
				JS_ThrowInternalError(context_, "invalied parent class id %u", parent_id);
			}
			JS_FreeValue(context_, parent);
		}
	}
	static JSClassID class_id_;
};


template<class T>
JSClassID Class<T>::class_id_ = 0;

template<class T>
Class<T> Module::ExportClass(const char* name) {
	return Class<T>(context_, this, name);
}


class Promise {
public:
	Promise(JSContext* ctx)
		:ctx_(ctx)
	{
		JSValue rfuncs[2];
		promise_ = JS_NewPromiseCapability(ctx, rfuncs);
		if (JS_IsException(promise_)) {
			rfuncs_[0] = JS_UNDEFINED;
			rfuncs_[1] = JS_UNDEFINED;
		}
		else {
			rfuncs_[0] = rfuncs[0];
			rfuncs_[1] = rfuncs[1];
		}
	}

	Promise(Context& ctx)
		:Promise(ctx.context())
	{
	}

	~Promise() {
		JS_FreeValue(ctx_, rfuncs_[0]);
		JS_FreeValue(ctx_, rfuncs_[1]);
		JS_FreeValue(ctx_, promise_);
	}

	Value promise() {
		return Value(ctx_, promise_);
	}

	enum Type {
		kResolve,
		kReject,
	};

	void Mark(JS_MarkFunc* mark_func) {
		JSRuntime* runtime = JS_GetRuntime(ctx_);
		JS_MarkValue(runtime, promise_, mark_func);
		JS_MarkValue(runtime, rfuncs_[0], mark_func);
		JS_MarkValue(runtime, rfuncs_[1], mark_func);
	}

	Value Resolve(Value val) {
		JSValue argv = val;
		JSValue ret = JS_Call(ctx_, rfuncs_[kResolve], JS_UNDEFINED, 1, &argv);
		return Value(ctx_, std::move(ret));
	}

	Value Resolve() {
		JSValue ret = JS_Call(ctx_, rfuncs_[kResolve], JS_UNDEFINED, 0, NULL);
		return Value(ctx_, std::move(ret));
	}

	Value Reject(Value val) {
		JSValue argv = val;
		JSValue ret = JS_Call(ctx_, rfuncs_[kReject], JS_UNDEFINED, 1, &argv);
		return Value(ctx_, std::move(ret));
	}

	Value Reject() {
		JSValue ret = JS_Call(ctx_, rfuncs_[kReject], JS_UNDEFINED, 0, NULL);
		return Value(ctx_, std::move(ret));
	}

	void Clear() {
		JS_FreeValue(ctx_, rfuncs_[0]);
		rfuncs_[0] = JS_UNDEFINED;
		JS_FreeValue(ctx_, rfuncs_[1]);
		rfuncs_[1] = JS_UNDEFINED;
		JS_FreeValue(ctx_, promise_);
		promise_ = JS_UNDEFINED;
	}

	JSContext* context() { return ctx_; }
private:
	QJS_DISALLOW_COPY_AND_ASSIGN(Promise);
	JSContext* ctx_;
	JSValue promise_;
	JSValue rfuncs_[2];
};


}//namespace

#include "qjs.inl"
