#include <limits.h>
//
// Created by lizhi on 2021/2/21.
//
#include "lualib.h"
#include "lauxlib.h"
#include <stdlib.h>
#include <stdio.h>
#include "luajava.h"

#define LUA_JAVA_MODULE_NAME "luajava"
#define PACKAGE_NAME "top/lizhistudio/androidlua"
#define TYPE_ERROR_CLASS_NAME PACKAGE_NAME "/exception/LuaTypeError"
#define INVOKE_ERROR_CLASS_NAME PACKAGE_NAME "/exception/LuaInvokeError"
#define JAVA_INTERFACE_WRAP_CLASS_NAME PACKAGE_NAME "/JavaObjectWrapper"
#define LUA_CONTEXT_CLASS_NAME PACKAGE_NAME "/LuaContextImplement"
#define LUA_HANDLER_CLASS_NAME PACKAGE_NAME "/LuaHandler"
#define PUSH_THROWABLE_ERROR "push java throwable error"
#define THROWABLE_CLASS_NAME "java/lang/Throwable"
#define DEBUG_INFO_CLASS_NAME PACKAGE_NAME "/DebugInfo"

#define JAVA_WRAPPER_OBJECT_NAME "JavaObjectWrapper"
#define LUA_HANDLER_NAME "LuaHandler"
#define GLOBAL(env, obj) (*env)->NewGlobalRef(env,obj)
#define FreeLocal(env,obj)  (*env)->DeleteLocalRef(env,obj)
#define JavaFindClass(env,str) (*env)->FindClass(env,str)
#define FreeGlobal(env,obj) (*env)->DeleteGlobalRef(env,obj)


static jclass LuaContextClass = NULL;
static jclass LuaHandlerClass = NULL;
static jclass LuaTypeErrorClass = NULL;
static jclass LuaInvokeErrorClass = NULL;
static jclass JavaObjectWrapperClass = NULL;
static jclass ThrowableClass = NULL;

static jclass DebugInfoClass = NULL;
static jfieldID DebugInfoNativePointer = NULL;

static jmethodID IndexMethodID = NULL;
static jmethodID NewIndexMethodID = NULL;
static jmethodID CallMethodID = NULL;
static jmethodID EqualMethodID = NULL;
static jmethodID LenMethodID = NULL;


static jmethodID CallMethodMethodID = NULL;


static jmethodID GetJavaObjectWrapperID = NULL;
static jmethodID PushJavaObjectWrapperID = NULL;
static jmethodID PushJavaObjectID = NULL;
static jmethodID RemoveJavaObjectWrapperID = NULL;

static jmethodID LuaHandlerCallID = NULL;




JNIEXPORT jint JNI_OnLoad(JavaVM * vm, void * reserved)
{
#define findGlobalClass(env,name) GLOBAL(env,JavaFindClass(env,name))
    JNIEnv * env = NULL;
    if ((*vm)->GetEnv(vm,(void**)&env, JNI_VERSION_1_6) != JNI_OK)
        return -1;
    LuaHandlerClass = findGlobalClass(env,LUA_HANDLER_CLASS_NAME);
    LuaTypeErrorClass = findGlobalClass(env, TYPE_ERROR_CLASS_NAME);
    LuaContextClass = findGlobalClass(env, LUA_CONTEXT_CLASS_NAME);
    LuaInvokeErrorClass = findGlobalClass(env, INVOKE_ERROR_CLASS_NAME);
    JavaObjectWrapperClass = findGlobalClass(env, JAVA_INTERFACE_WRAP_CLASS_NAME);
    ThrowableClass = findGlobalClass(env,THROWABLE_CLASS_NAME);
    DebugInfoClass = findGlobalClass(env,DEBUG_INFO_CLASS_NAME);

    CallMethodID = (*env)->GetMethodID(env,JavaObjectWrapperClass,"__call", "(Ltop/lizhistudio/androidlua/LuaContext;)I");
    NewIndexMethodID = (*env)->GetMethodID(env,JavaObjectWrapperClass,"__newIndex",
                                           "(Ltop/lizhistudio/androidlua/LuaContext;)I");
    IndexMethodID = (*env)->GetMethodID(env,JavaObjectWrapperClass,"__index",
                                        "(Ltop/lizhistudio/androidlua/LuaContext;)I");
    EqualMethodID = (*env)->GetMethodID(env,JavaObjectWrapperClass,"__equal",
                                        "(Ltop/lizhistudio/androidlua/LuaContext;)I");
    LenMethodID = (*env)->GetMethodID(env,JavaObjectWrapperClass,"__len",
                                      "(Ltop/lizhistudio/androidlua/LuaContext;)I");


    CallMethodMethodID = (*env)->GetMethodID(env, JavaObjectWrapperClass, "callMethod",
                                             "(Ltop/lizhistudio/androidlua/LuaContext;Ljava/lang/String;)I");

    LuaHandlerCallID = (*env)->GetMethodID(env,LuaHandlerClass,"onExecute", "(Ltop/lizhistudio/androidlua/LuaContext;)I");

    GetJavaObjectWrapperID = (*env)->GetMethodID(env,LuaContextClass,"getJavaObject",
                                                 "(J)Ljava/lang/Object;");
    PushJavaObjectWrapperID = (*env)->GetMethodID(env,LuaContextClass,"cacheJavaObject",
                                                  "(JLjava/lang/Object;)V");
    RemoveJavaObjectWrapperID = (*env)->GetMethodID(env,LuaContextClass,"removeJavaObject", "(J)V");
    PushJavaObjectID = (*env)->GetMethodID(env,LuaContextClass,"push", "(Ljava/lang/Class;Ljava/lang/Object;)V");

    DebugInfoNativePointer = (*env)->GetFieldID(env,DebugInfoClass,"nativePrint", "J");
    return  JNI_VERSION_1_6;
}

JNIEXPORT void JNI_OnUnload(JavaVM* vm, void* reserved){
    JNIEnv * env = NULL;
    if ((*vm)->GetEnv(vm,(void**)&env, JNI_VERSION_1_6) != JNI_OK)
        return ;
    FreeGlobal(env,LuaContextClass);
    FreeGlobal(env,LuaInvokeErrorClass);
    FreeGlobal(env,LuaTypeErrorClass);
    FreeGlobal(env, JavaObjectWrapperClass);
    FreeGlobal(env,ThrowableClass);
}


static void throwTypeError(JNIEnv *env, lua_State*L,int index,int exception)
{
    char buffer[1024];
    const char* exceptionName = lua_typename(L,exception);
    const char* nowName = lua_typename(L,lua_type(L,index));
    sprintf(buffer,"luaState %p  index %d exception type %s now type %s",L,index,exceptionName,nowName);
    (*env)->ThrowNew(env, LuaTypeErrorClass,buffer);
}

static int pushJavaThrowable(JNIEnv* env,lua_State*L,jthrowable throwable)
{
    LuaExtension *extension = GetLuaExtension(L);
    (*env)->CallVoidMethod(env,extension->context,PushJavaObjectID,ThrowableClass,throwable);
    if ((*env)->ExceptionCheck(env))
    {
        (*env)->ExceptionDescribe(env);
        (*env)->ExceptionClear(env);
        return 0;
    }
    return 1;
}

static int catchAndPushJavaThrowable(JNIEnv *env, lua_State*L)
{
    jthrowable throwable = (*env)->ExceptionOccurred(env);
    if (throwable != NULL)
    {
        (*env)->ExceptionDescribe(env);
        (*env)->ExceptionClear(env);
        if (!pushJavaThrowable(env,L,throwable))
            lua_pushstring(L,PUSH_THROWABLE_ERROR);
        (*env)->DeleteLocalRef(env,throwable);
        return 1;
    }
    return 0;

}


JNIEXPORT jlong JNICALL
Java_top_lizhistudio_androidlua_LuaJava_toInteger(JNIEnv *env, jclass clazz, jlong native_lua,
                                               jint index) {
    lua_State *L =toLuaState(native_lua);
    if (lua_isnumber(L,index))
    {
        return (jlong)lua_tointeger(L,index);
    } else{
        throwTypeError(env,L,index,LUA_TNUMBER);
    }
    return 0;
}

JNIEXPORT jdouble JNICALL
Java_top_lizhistudio_androidlua_LuaJava_toNumber(JNIEnv *env, jclass clazz, jlong native_lua,
                                              jint index) {
    lua_State *L =toLuaState(native_lua);
    if (lua_isnumber(L,index))
    {
        return (jdouble)lua_tonumber(L,index);
    }else{
        throwTypeError(env,L,index,LUA_TNUMBER);
    }
    return 0;
}

JNIEXPORT jboolean JNICALL
Java_top_lizhistudio_androidlua_LuaJava_toBoolean(JNIEnv *env, jclass clazz, jlong native_lua,
                                               jint index) {
    lua_State *L =toLuaState(native_lua);
    return lua_toboolean(L,index);
}

JNIEXPORT jbyteArray JNICALL
Java_top_lizhistudio_androidlua_LuaJava_toBytes(JNIEnv *env, jclass clazz, jlong native_lua,
                                             jint index) {
    lua_State *L =toLuaState(native_lua);
    if(lua_isstring(L,index))
    {
        size_t len = 0;
        const char* str = lua_tolstring(L,index,&len);
        jbyteArray result = (*env)->NewByteArray(env,len);
        if ((*env)->ExceptionCheck(env))
            return NULL;
        (*env)->SetByteArrayRegion(env,result,0,len,(jbyte*)str);
        return result;
    } else{
        throwTypeError(env,L,index,LUA_TSTRING);
    }
    return NULL;
}

JNIEXPORT jstring JNICALL
Java_top_lizhistudio_androidlua_LuaJava_toString(JNIEnv *env, jclass clazz, jlong native_lua,
                                              jint index) {
    lua_State *L =toLuaState(native_lua);
    if(lua_isstring(L,index))
    {
        size_t len = 0;
        const char* str = lua_tolstring(L,index,&len);
        jstring result = (*env)->NewStringUTF(env,str);
        return result;
    } else{
        throwTypeError(env,L,index,LUA_TSTRING);
    }
    return NULL;
}

JNIEXPORT void JNICALL
Java_top_lizhistudio_androidlua_LuaJava_push__JZ(JNIEnv *env, jclass clazz, jlong native_lua,
                                              jboolean v) {
    lua_pushboolean(toLuaState(native_lua),v);
}

JNIEXPORT void JNICALL
Java_top_lizhistudio_androidlua_LuaJava_push__J_3B(JNIEnv *env, jclass clazz, jlong native_lua,
                                                jbyteArray v) {
    lua_State *L =toLuaState(native_lua);
    jboolean isCopy = 0;
    jbyte * bytes = (*env)->GetByteArrayElements(env,v,&isCopy);
    jsize size = (*env)->GetArrayLength(env,v);
    lua_pushlstring(L,(const char*)bytes,size);
    (*env)->ReleaseByteArrayElements(env,v,bytes,0);
}

JNIEXPORT void JNICALL
Java_top_lizhistudio_androidlua_LuaJava_pushNil(JNIEnv *env, jclass clazz, jlong native_lua) {
    lua_pushnil(toLuaState(native_lua));
}

JNIEXPORT void JNICALL
Java_top_lizhistudio_androidlua_LuaJava_push__JJ(JNIEnv *env, jclass clazz, jlong native_lua,
                                              jlong v) {
    lua_pushinteger(toLuaState(native_lua),v);
}

JNIEXPORT void JNICALL
Java_top_lizhistudio_androidlua_LuaJava_push__JD(JNIEnv *env, jclass clazz, jlong native_lua,
                                              jdouble v) {
    lua_pushnumber(toLuaState(native_lua),v);
}






JNIEXPORT jint JNICALL
Java_top_lizhistudio_androidlua_LuaJava_type(JNIEnv *env, jclass clazz, jlong native_lua, jint index) {
    return lua_type(toLuaState(native_lua),index);
}

JNIEXPORT jboolean JNICALL
Java_top_lizhistudio_androidlua_LuaJava_isInteger(JNIEnv *env, jclass clazz, jlong native_value,
                                               jint index) {
    return lua_isinteger(toLuaState(native_value),index);
}


static jobject getJavaObject(LuaExtension *extension, lua_State*L, int index)
{
    JNIEnv *env = extension->env;
    jlong* pId = (jlong *)lua_touserdata(L,index);
    return (*env)->CallObjectMethod(env,extension->context,GetJavaObjectWrapperID,*pId);
}



JNIEXPORT void JNICALL
Java_top_lizhistudio_androidlua_LuaJava_setGlobal(JNIEnv *env, jclass clazz, jlong native_lua,
                                               jstring key) {
    lua_State *L = toLuaState(native_lua);
    SetJNIEnv(L,env);
    jboolean isCopy = 0;
    const char* globalKey =(*env)->GetStringUTFChars(env,key,&isCopy);
    lua_setglobal(L,globalKey);
    (*env)->ReleaseStringUTFChars(env,key,globalKey);
}




static int javaObjectMethod(lua_State*L)
{
    LuaExtension *extension = GetLuaExtension(L);
    JNIEnv *env = extension->env;
    const char* methodName = lua_tostring(L,lua_upvalueindex(2));
    jstring jMethodName = (*env)->NewStringUTF(env,methodName);
    jobject object = getJavaObject(extension, L, lua_upvalueindex(1));
    jint result = (*env)->CallIntMethod(env,object,CallMethodMethodID,extension->context,jMethodName);
    FreeLocal(env,jMethodName);
    FreeLocal(env,object);
    if (catchAndPushJavaThrowable(env,L) || result == -1)
    {
        lua_error(L);
    }
    return result;
}

static int javaObjectDestroy(lua_State*L)
{
    LuaExtension *extension = GetLuaExtension(L);
    JNIEnv *env = extension->env;
    jlong* pId = (jlong *)lua_touserdata(L,1);
    if (*pId)
    {
        (*env)->CallVoidMethod(env,extension->context,RemoveJavaObjectWrapperID,*pId);
        *pId = 0;
    }
    return 0;
}


static int callJavaObject(lua_State*L,jmethodID id,int index)
{
    LuaExtension *extension = GetLuaExtension(L);
    jobject object = getJavaObject(extension, L, index);
    JNIEnv *env = extension->env;
    int result = (*env)->CallIntMethod(env,object,id,extension->context);
    FreeLocal(env,object);
    if (catchAndPushJavaThrowable(env,L))
        lua_error(L);
    return result;
}

#define JAVA_METHOD_DEFINE(n) static int javaObject##n(lua_State*L)\
{\
    return callJavaObject(L,n##MethodID,1);\
}

JAVA_METHOD_DEFINE(Index)
JAVA_METHOD_DEFINE(NewIndex)
JAVA_METHOD_DEFINE(Call)
JAVA_METHOD_DEFINE(Len)
JAVA_METHOD_DEFINE(Equal)


static int destroyJavaObject(lua_State*L)
{
    if (luaL_testudata(L, 1, JAVA_WRAPPER_OBJECT_NAME))
    {
        javaObjectDestroy(L);
        lua_pushboolean(L,1);
    } else
        lua_pushboolean(L,0);
    return 1;
}


static int luaopen_luajava(lua_State*L)
{
    if (luaL_newmetatable(L, JAVA_WRAPPER_OBJECT_NAME))
    {
        luaL_Reg methods[] = {
                {"__gc",javaObjectDestroy},
                {"__index",javaObjectIndex},
                {"__newindex",javaObjectNewIndex},
                {"__len",javaObjectLen},
                {"__eq",javaObjectEqual},
                {"__call",javaObjectCall},
                {NULL,NULL}
        };
        luaL_setfuncs(L,methods,0);
    }
    lua_pop(L,1);

    if (luaL_newmetatable(L,LUA_HANDLER_NAME))
    {
        luaL_Reg methods[] = {
                {"__gc",javaObjectDestroy},
                {NULL,NULL}
        };
        luaL_setfuncs(L,methods,0);
    }
    lua_pop(L,1);

    luaL_Reg methods[] = {
            {"destroy",destroyJavaObject},
            {NULL,NULL}
    };
    luaL_newlib(L,methods);
    return 1;
}

JNIEXPORT jlong JNICALL
Java_top_lizhistudio_androidlua_LuaJava_newLuaState(JNIEnv *env, jclass clazz, jobject context) {
    LuaExtension * extension = malloc(sizeof(LuaExtension));
    extension->context = (*env)->NewWeakGlobalRef(env,context);
    extension->env = env;
    lua_State *L = luaL_newstate();
    SetLuaExtension(L,extension);
    luaL_requiref(L, LUA_JAVA_MODULE_NAME, luaopen_luajava, 1);
    luaL_openlibs(L);
    lua_pop(L,1);
    return (jlong)L;
}

JNIEXPORT void JNICALL
Java_top_lizhistudio_androidlua_LuaJava_closeLuaState(JNIEnv *env, jclass clazz, jlong native_lua) {
    lua_State *L = toLuaState(native_lua);
    LuaExtension * extension = GetLuaExtension(native_lua);
    extension->env = env;
    lua_close(L);
    (*env)->DeleteWeakGlobalRef(env,extension->context);
    free(extension);
}


static void pushJavaObject(JNIEnv *env, jlong native_lua, jobject object_wrapper)
{
    lua_State *L = toLuaState(native_lua);
    LuaExtension *extension = GetLuaExtension(L);
    extension->env = env;
    jlong* pId = lua_newuserdata(L,sizeof(jlong));
    *pId = (jlong)lua_topointer(L,-1);
    (*env)->CallVoidMethod(env,extension->context,PushJavaObjectWrapperID,*pId,object_wrapper);
}


static int luaHandlerCall(lua_State*L)
{
    return callJavaObject(L,LuaHandlerCallID,lua_upvalueindex(1));
}

JNIEXPORT void JNICALL
Java_top_lizhistudio_androidlua_LuaJava_push__JLtop_lizhistudio_androidlua_LuaHandler_2(JNIEnv *env,
                                                                                        jclass clazz,
                                                                                        jlong native_lua,
                                                                                        jobject lua_handler) {
    pushJavaObject(env, native_lua, lua_handler);
    luaL_setmetatable(toLuaState(native_lua), LUA_HANDLER_NAME);
    lua_pushcclosure(toLuaState(native_lua),luaHandlerCall,1);
}


JNIEXPORT void JNICALL
Java_top_lizhistudio_androidlua_LuaJava_push__JLtop_lizhistudio_androidlua_JavaObjectWrapper_2(
        JNIEnv *env, jclass clazz, jlong native_lua, jobject object_wrapper) {
    pushJavaObject(env, native_lua, object_wrapper);
    luaL_setmetatable(toLuaState(native_lua), JAVA_WRAPPER_OBJECT_NAME);
}

JNIEXPORT jobject JNICALL
Java_top_lizhistudio_androidlua_LuaJava_toJavaObject(JNIEnv *env, jclass clazz, jlong native_lua,
                                                  jint index) {
    lua_State *L = toLuaState(native_lua);
    LuaExtension * extension = GetLuaExtension(L);
    extension->env = env;
    jlong * pId = (jlong*)luaL_testudata(L, index, JAVA_WRAPPER_OBJECT_NAME);
    return pId ? getJavaObject(extension, L, index) : NULL ;
}

JNIEXPORT jboolean JNICALL
Java_top_lizhistudio_androidlua_LuaJava_isJavaObjectWrapper(JNIEnv *env, jclass clazz,
                                                         jlong native_lua, jint index) {
    lua_State *L = toLuaState(native_lua);
    return luaL_testudata(L, index, JAVA_WRAPPER_OBJECT_NAME) > 0;
}

JNIEXPORT jint JNICALL
Java_top_lizhistudio_androidlua_LuaJava_getTop(JNIEnv *env, jclass clazz, jlong native_lua) {
    return lua_gettop(toLuaState(native_lua));
}

JNIEXPORT void JNICALL
Java_top_lizhistudio_androidlua_LuaJava_setTop(JNIEnv *env, jclass clazz, jlong native_lua,
                                            jint index) {
    lua_settop(toLuaState(native_lua),index);
}

JNIEXPORT void JNICALL
Java_top_lizhistudio_androidlua_LuaJava_pushJavaObjectMethod(JNIEnv *env, jclass clazz,
                                                             jlong native_lua) {
    lua_State *L = toLuaState(native_lua);
    if (luaL_testudata(L, -2, JAVA_WRAPPER_OBJECT_NAME))
    {
        if (lua_isstring(L,-1))
        {
            lua_pushcclosure(L,javaObjectMethod,2);
        } else
            throwTypeError(env,L,-1,LUA_TSTRING);
    } else
        throwTypeError(env,L,-2,LUA_TUSERDATA);

}


static const char* getCodeMode(int t)
{
    const char* mode;
    if (t == 0)
        mode = "bt";
    else if(t == 1)
        mode = "t";
    else
        mode = "b";
    return mode;
}

JNIEXPORT jint JNICALL
Java_top_lizhistudio_androidlua_LuaJava_loadBuffer(JNIEnv *env, jclass clazz, jlong native_lua,
                                                   jbyteArray code, jstring chunk_name,
                                                   jint code_type) {
    jboolean isCopy = 0;
    jbyte* cCode = (*env)->GetByteArrayElements(env,code,&isCopy);
    jsize codeSize = (*env)->GetArrayLength(env,code);
    isCopy = 0;
    const char* chunkName = (*env)->GetStringUTFChars(env,chunk_name,&isCopy);
    lua_State *L = toLuaState(native_lua);
    SetJNIEnv(L,env);
    int result = luaL_loadbufferx(L, (const char*)cCode, codeSize, chunkName, getCodeMode(code_type));
    (*env)->ReleaseByteArrayElements(env,code,cCode,0);
    (*env)->ReleaseStringUTFChars(env,chunk_name,chunkName);
    return result;
}


JNIEXPORT jint JNICALL
Java_top_lizhistudio_androidlua_LuaJava_loadFile(JNIEnv *env, jclass clazz, jlong native_lua,
                                                 jstring file_name, jint code_type) {
    jboolean isCopy = 0;
    const char* codePath = (*env)->GetStringUTFChars(env,file_name,&isCopy);
    lua_State *L = toLuaState(native_lua);
    SetJNIEnv(L,env);
    int result = luaL_loadfilex(L, codePath, getCodeMode(code_type));
    (*env)->ReleaseStringUTFChars(env,file_name,codePath);
    return result;
}

JNIEXPORT jint JNICALL
Java_top_lizhistudio_androidlua_LuaJava_reference(JNIEnv *env, jclass clazz, jlong native_lua,
                                                  jint table_index) {
    return luaL_ref(toLuaState(native_lua),table_index);
}

JNIEXPORT void JNICALL
Java_top_lizhistudio_androidlua_LuaJava_unReference(JNIEnv *env, jclass clazz, jlong native_lua,
                                                    jint table_index, jint reference) {
    luaL_unref(toLuaState(native_lua),table_index,reference);
}

JNIEXPORT jint JNICALL
Java_top_lizhistudio_androidlua_LuaJava_pCall(JNIEnv *env, jclass clazz, jlong native_lua,
                                              jint arg_number, jint result_number,
                                              jint error_function_index) {
    lua_State *L = toLuaState(native_lua);
    SetJNIEnv(L,env);
    return lua_pcall(L,arg_number,result_number,error_function_index);
}

JNIEXPORT void JNICALL
Java_top_lizhistudio_androidlua_LuaJava_pop(JNIEnv *env, jclass clazz, jlong native_lua, jint n) {
    lua_pop(toLuaState(native_lua), n);
}


lua_Debug * getNativeDebugInfo(JNIEnv*env,jobject debugInfo)
{
    return (lua_Debug*) (*env)->GetLongField(env,debugInfo,DebugInfoNativePointer);
}


JNIEXPORT jboolean JNICALL
Java_top_lizhistudio_androidlua_LuaJava_getInfo(JNIEnv *env, jclass clazz, jlong native_lua,
                                                jstring what, jobject debug_info) {
    jboolean isCopy = 0;
    const char* cWhat = (*env)->GetStringUTFChars(env,what,&isCopy);
    lua_Debug* luaDebug = getNativeDebugInfo(env,debug_info);
    jboolean result = lua_getinfo(toLuaState(native_lua),cWhat,luaDebug);
    (*env)->ReleaseStringUTFChars(env,what,cWhat);
    return result;
}

JNIEXPORT jboolean JNICALL
Java_top_lizhistudio_androidlua_LuaJava_getStack(JNIEnv *env, jclass clazz, jlong native_lua,
                                                 jint level, jobject debug_info) {
    lua_Debug* luaDebug = getNativeDebugInfo(env,debug_info);
    return lua_getstack(toLuaState(native_lua),level,luaDebug);
}

JNIEXPORT jlong JNICALL
Java_top_lizhistudio_androidlua_DebugInfo_newDebugInfo(JNIEnv *env, jclass clazz) {
    return (jlong)malloc(sizeof(lua_Debug));
}

JNIEXPORT void JNICALL
Java_top_lizhistudio_androidlua_DebugInfo_release(JNIEnv *env, jclass clazz, jlong native_print) {
    free((void*)native_print);
}

JNIEXPORT jstring JNICALL
Java_top_lizhistudio_androidlua_DebugInfo_getName(JNIEnv *env, jclass clazz, jlong native_print) {
    lua_Debug* luaDebug= (lua_Debug*)native_print;
    if (luaDebug->name == NULL)
        return NULL;
    return (*env)->NewStringUTF(env,luaDebug->name);
}

JNIEXPORT jstring JNICALL
Java_top_lizhistudio_androidlua_DebugInfo_getNameWhat(JNIEnv *env, jclass clazz,
                                                      jlong native_print) {
    lua_Debug* luaDebug= (lua_Debug*)native_print;
    if (luaDebug->namewhat == NULL)
        return NULL;
    return (*env)->NewStringUTF(env,luaDebug->namewhat);
}

JNIEXPORT jstring JNICALL
Java_top_lizhistudio_androidlua_DebugInfo_getSource(JNIEnv *env, jclass clazz, jlong native_print) {
    lua_Debug* luaDebug= (lua_Debug*)native_print;
    if (luaDebug->source == NULL)
        return NULL;
    return (*env)->NewStringUTF(env,luaDebug->source);
}

JNIEXPORT jstring JNICALL
Java_top_lizhistudio_androidlua_DebugInfo_getShortSource(JNIEnv *env, jclass clazz,
                                                         jlong native_print) {
    lua_Debug* luaDebug= (lua_Debug*)native_print;
    return (*env)->NewStringUTF(env,luaDebug->short_src);
}

JNIEXPORT jstring JNICALL
Java_top_lizhistudio_androidlua_DebugInfo_getWhat(JNIEnv *env, jclass clazz, jlong native_print) {
    lua_Debug* luaDebug= (lua_Debug*)native_print;
    if (luaDebug->what == NULL)
        return NULL;
    return (*env)->NewStringUTF(env,luaDebug->what);
}

JNIEXPORT jint JNICALL
Java_top_lizhistudio_androidlua_DebugInfo_getCurrentLine(JNIEnv *env, jclass clazz,
                                                         jlong native_print) {
    lua_Debug* luaDebug= (lua_Debug*)native_print;
    return luaDebug->currentline;
}

JNIEXPORT jint JNICALL
Java_top_lizhistudio_androidlua_DebugInfo_getLastLineDefined(JNIEnv *env, jclass clazz,
                                                             jlong native_print) {
    lua_Debug* luaDebug= (lua_Debug*)native_print;
    return luaDebug->lastlinedefined;
}

JNIEXPORT jint JNICALL
Java_top_lizhistudio_androidlua_DebugInfo_getLineDefined(JNIEnv *env, jclass clazz,
                                                         jlong native_print) {
    lua_Debug* luaDebug= (lua_Debug*)native_print;
    return luaDebug->linedefined;
}

JNIEXPORT jint JNICALL
Java_top_lizhistudio_androidlua_DebugInfo_getParamsSum(JNIEnv *env, jclass clazz,
                                                       jlong native_print) {
    lua_Debug* luaDebug= (lua_Debug*)native_print;
    return luaDebug->nparams;
}

JNIEXPORT jint JNICALL
Java_top_lizhistudio_androidlua_DebugInfo_getUpValueSum(JNIEnv *env, jclass clazz,
                                                        jlong native_print) {
    lua_Debug* luaDebug= (lua_Debug*)native_print;
    return luaDebug->nups;
}

JNIEXPORT jboolean JNICALL
Java_top_lizhistudio_androidlua_DebugInfo_isTailCall(JNIEnv *env, jclass clazz,
                                                     jlong native_print) {
    lua_Debug* luaDebug= (lua_Debug*)native_print;
    return luaDebug->istailcall;
}

JNIEXPORT jboolean JNICALL
Java_top_lizhistudio_androidlua_DebugInfo_isVarArg(JNIEnv *env, jclass clazz, jlong native_print) {
    lua_Debug* luaDebug= (lua_Debug*)native_print;
    return luaDebug->isvararg;
}