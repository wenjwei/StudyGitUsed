﻿using System;
using com.tencent.pandora;
using System.Collections.Generic;
public class Lua_System_Object : LuaObject {
	[com.tencent.pandora.MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
	static public int constructor(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]System.Object.constructor");
			#endif
			System.Object o;
			o=new System.Object();
			pushValue(l,true);
			pushValue(l,o);
			return 2;
		}
		catch(Exception e) {
			return error(l,e);
		}
		#if PANDORA_PROFILE
		finally {
			endSample();
		}
		#endif
	}
	[com.tencent.pandora.MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
	static public int ReferenceEquals_s(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]System.Object.ReferenceEquals");
			#endif
			System.Object a1;
			checkType(l,1,out a1);
			System.Object a2;
			checkType(l,2,out a2);
			var ret=System.Object.ReferenceEquals(a1,a2);
			pushValue(l,true);
			pushValue(l,ret);
			return 2;
		}
		catch(Exception e) {
			return error(l,e);
		}
		#if PANDORA_PROFILE
		finally {
			endSample();
		}
		#endif
	}
	static public void reg(IntPtr l) {
		getTypeTable(l,"System.Object");
		addMember(l,ReferenceEquals_s);
		createTypeMetatable(l,constructor, typeof(System.Object));
	}
}
