﻿using System;
using com.tencent.pandora;
using System.Collections.Generic;
public class Lua_UnityEngine_Component : LuaObject {
	[com.tencent.pandora.MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
	static public int constructor(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Component.constructor");
			#endif
			UnityEngine.Component o;
			o=new UnityEngine.Component();
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
	static public int GetComponent(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Component.GetComponent");
			#endif
			int argc = LuaDLL.pua_gettop(l);
			if(matchType(l,argc,2,typeof(string))){
				UnityEngine.Component self=(UnityEngine.Component)checkSelf(l);
				System.String a1;
				checkType(l,2,out a1);
				var ret=self.GetComponent(a1);
				pushValue(l,true);
				pushValue(l,ret);
				return 2;
			}
			else if(matchType(l,argc,2,typeof(System.Type))){
				UnityEngine.Component self=(UnityEngine.Component)checkSelf(l);
				System.Type a1;
				checkType(l,2,out a1);
				var ret=self.GetComponent(a1);
				pushValue(l,true);
				pushValue(l,ret);
				return 2;
			}
			pushValue(l,false);
			LuaDLL.pua_pushstring(l,"No matched override function to call");
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
	static public int GetComponentInChildren(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Component.GetComponentInChildren");
			#endif
			int argc = LuaDLL.pua_gettop(l);
			if(argc==2){
				UnityEngine.Component self=(UnityEngine.Component)checkSelf(l);
				System.Type a1;
				checkType(l,2,out a1);
				var ret=self.GetComponentInChildren(a1);
				pushValue(l,true);
				pushValue(l,ret);
				return 2;
			}
			else if(argc==3){
				UnityEngine.Component self=(UnityEngine.Component)checkSelf(l);
				System.Type a1;
				checkType(l,2,out a1);
				System.Boolean a2;
				checkType(l,3,out a2);
				var ret=self.GetComponentInChildren(a1,a2);
				pushValue(l,true);
				pushValue(l,ret);
				return 2;
			}
			pushValue(l,false);
			LuaDLL.pua_pushstring(l,"No matched override function to call");
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
	static public int GetComponentsInChildren(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Component.GetComponentsInChildren");
			#endif
			int argc = LuaDLL.pua_gettop(l);
			if(argc==2){
				UnityEngine.Component self=(UnityEngine.Component)checkSelf(l);
				System.Type a1;
				checkType(l,2,out a1);
				var ret=self.GetComponentsInChildren(a1);
				pushValue(l,true);
				pushValue(l,ret);
				return 2;
			}
			else if(argc==3){
				UnityEngine.Component self=(UnityEngine.Component)checkSelf(l);
				System.Type a1;
				checkType(l,2,out a1);
				System.Boolean a2;
				checkType(l,3,out a2);
				var ret=self.GetComponentsInChildren(a1,a2);
				pushValue(l,true);
				pushValue(l,ret);
				return 2;
			}
			pushValue(l,false);
			LuaDLL.pua_pushstring(l,"No matched override function to call");
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
	static public int GetComponentInParent(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Component.GetComponentInParent");
			#endif
			UnityEngine.Component self=(UnityEngine.Component)checkSelf(l);
			System.Type a1;
			checkType(l,2,out a1);
			var ret=self.GetComponentInParent(a1);
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
	[com.tencent.pandora.MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
	static public int GetComponentsInParent(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Component.GetComponentsInParent");
			#endif
			int argc = LuaDLL.pua_gettop(l);
			if(argc==2){
				UnityEngine.Component self=(UnityEngine.Component)checkSelf(l);
				System.Type a1;
				checkType(l,2,out a1);
				var ret=self.GetComponentsInParent(a1);
				pushValue(l,true);
				pushValue(l,ret);
				return 2;
			}
			else if(argc==3){
				UnityEngine.Component self=(UnityEngine.Component)checkSelf(l);
				System.Type a1;
				checkType(l,2,out a1);
				System.Boolean a2;
				checkType(l,3,out a2);
				var ret=self.GetComponentsInParent(a1,a2);
				pushValue(l,true);
				pushValue(l,ret);
				return 2;
			}
			pushValue(l,false);
			LuaDLL.pua_pushstring(l,"No matched override function to call");
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
	static public int GetComponents(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Component.GetComponents");
			#endif
			int argc = LuaDLL.pua_gettop(l);
			if(argc==2){
				UnityEngine.Component self=(UnityEngine.Component)checkSelf(l);
				System.Type a1;
				checkType(l,2,out a1);
				var ret=self.GetComponents(a1);
				pushValue(l,true);
				pushValue(l,ret);
				return 2;
			}
			else if(argc==3){
				UnityEngine.Component self=(UnityEngine.Component)checkSelf(l);
				System.Type a1;
				checkType(l,2,out a1);
				System.Collections.Generic.List<UnityEngine.Component> a2;
				checkType(l,3,out a2);
				self.GetComponents(a1,a2);
				pushValue(l,true);
				return 1;
			}
			pushValue(l,false);
			LuaDLL.pua_pushstring(l,"No matched override function to call");
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
	static public int CompareTag(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Component.CompareTag");
			#endif
			UnityEngine.Component self=(UnityEngine.Component)checkSelf(l);
			System.String a1;
			checkType(l,2,out a1);
			var ret=self.CompareTag(a1);
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
	[com.tencent.pandora.MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
	static public int SendMessageUpwards(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Component.SendMessageUpwards");
			#endif
			int argc = LuaDLL.pua_gettop(l);
			if(argc==2){
				UnityEngine.Component self=(UnityEngine.Component)checkSelf(l);
				System.String a1;
				checkType(l,2,out a1);
				self.SendMessageUpwards(a1);
				pushValue(l,true);
				return 1;
			}
			else if(matchType(l,argc,2,typeof(string),typeof(UnityEngine.SendMessageOptions))){
				UnityEngine.Component self=(UnityEngine.Component)checkSelf(l);
				System.String a1;
				checkType(l,2,out a1);
				UnityEngine.SendMessageOptions a2;
				checkEnum(l,3,out a2);
				self.SendMessageUpwards(a1,a2);
				pushValue(l,true);
				return 1;
			}
			else if(matchType(l,argc,2,typeof(string),typeof(System.Object))){
				UnityEngine.Component self=(UnityEngine.Component)checkSelf(l);
				System.String a1;
				checkType(l,2,out a1);
				System.Object a2;
				checkType(l,3,out a2);
				self.SendMessageUpwards(a1,a2);
				pushValue(l,true);
				return 1;
			}
			else if(argc==4){
				UnityEngine.Component self=(UnityEngine.Component)checkSelf(l);
				System.String a1;
				checkType(l,2,out a1);
				System.Object a2;
				checkType(l,3,out a2);
				UnityEngine.SendMessageOptions a3;
				checkEnum(l,4,out a3);
				self.SendMessageUpwards(a1,a2,a3);
				pushValue(l,true);
				return 1;
			}
			pushValue(l,false);
			LuaDLL.pua_pushstring(l,"No matched override function to call");
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
	static public int SendMessage(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Component.SendMessage");
			#endif
			int argc = LuaDLL.pua_gettop(l);
			if(argc==2){
				UnityEngine.Component self=(UnityEngine.Component)checkSelf(l);
				System.String a1;
				checkType(l,2,out a1);
				self.SendMessage(a1);
				pushValue(l,true);
				return 1;
			}
			else if(matchType(l,argc,2,typeof(string),typeof(UnityEngine.SendMessageOptions))){
				UnityEngine.Component self=(UnityEngine.Component)checkSelf(l);
				System.String a1;
				checkType(l,2,out a1);
				UnityEngine.SendMessageOptions a2;
				checkEnum(l,3,out a2);
				self.SendMessage(a1,a2);
				pushValue(l,true);
				return 1;
			}
			else if(matchType(l,argc,2,typeof(string),typeof(System.Object))){
				UnityEngine.Component self=(UnityEngine.Component)checkSelf(l);
				System.String a1;
				checkType(l,2,out a1);
				System.Object a2;
				checkType(l,3,out a2);
				self.SendMessage(a1,a2);
				pushValue(l,true);
				return 1;
			}
			else if(argc==4){
				UnityEngine.Component self=(UnityEngine.Component)checkSelf(l);
				System.String a1;
				checkType(l,2,out a1);
				System.Object a2;
				checkType(l,3,out a2);
				UnityEngine.SendMessageOptions a3;
				checkEnum(l,4,out a3);
				self.SendMessage(a1,a2,a3);
				pushValue(l,true);
				return 1;
			}
			pushValue(l,false);
			LuaDLL.pua_pushstring(l,"No matched override function to call");
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
	static public int BroadcastMessage(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Component.BroadcastMessage");
			#endif
			int argc = LuaDLL.pua_gettop(l);
			if(argc==2){
				UnityEngine.Component self=(UnityEngine.Component)checkSelf(l);
				System.String a1;
				checkType(l,2,out a1);
				self.BroadcastMessage(a1);
				pushValue(l,true);
				return 1;
			}
			else if(matchType(l,argc,2,typeof(string),typeof(UnityEngine.SendMessageOptions))){
				UnityEngine.Component self=(UnityEngine.Component)checkSelf(l);
				System.String a1;
				checkType(l,2,out a1);
				UnityEngine.SendMessageOptions a2;
				checkEnum(l,3,out a2);
				self.BroadcastMessage(a1,a2);
				pushValue(l,true);
				return 1;
			}
			else if(matchType(l,argc,2,typeof(string),typeof(System.Object))){
				UnityEngine.Component self=(UnityEngine.Component)checkSelf(l);
				System.String a1;
				checkType(l,2,out a1);
				System.Object a2;
				checkType(l,3,out a2);
				self.BroadcastMessage(a1,a2);
				pushValue(l,true);
				return 1;
			}
			else if(argc==4){
				UnityEngine.Component self=(UnityEngine.Component)checkSelf(l);
				System.String a1;
				checkType(l,2,out a1);
				System.Object a2;
				checkType(l,3,out a2);
				UnityEngine.SendMessageOptions a3;
				checkEnum(l,4,out a3);
				self.BroadcastMessage(a1,a2,a3);
				pushValue(l,true);
				return 1;
			}
			pushValue(l,false);
			LuaDLL.pua_pushstring(l,"No matched override function to call");
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
	static public int get_transform(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Component.transform");
			#endif
			UnityEngine.Component self=(UnityEngine.Component)checkSelf(l);
			pushValue(l,true);
			pushValue(l,self.transform);
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
	static public int get_gameObject(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Component.gameObject");
			#endif
			UnityEngine.Component self=(UnityEngine.Component)checkSelf(l);
			pushValue(l,true);
			pushValue(l,self.gameObject);
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
	static public int get_tag(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Component.tag");
			#endif
			UnityEngine.Component self=(UnityEngine.Component)checkSelf(l);
			pushValue(l,true);
			pushValue(l,self.tag);
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
	static public int set_tag(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Component.tag");
			#endif
			UnityEngine.Component self=(UnityEngine.Component)checkSelf(l);
			string v;
			checkType(l,2,out v);
			self.tag=v;
			pushValue(l,true);
			return 1;
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
		getTypeTable(l,"UnityEngine.Component");
		addMember(l,GetComponent);
		addMember(l,GetComponentInChildren);
		addMember(l,GetComponentsInChildren);
		addMember(l,GetComponentInParent);
		addMember(l,GetComponentsInParent);
		addMember(l,GetComponents);
		addMember(l,CompareTag);
		addMember(l,SendMessageUpwards);
		addMember(l,SendMessage);
		addMember(l,BroadcastMessage);
		addMember(l,"transform",get_transform,null,true);
		addMember(l,"gameObject",get_gameObject,null,true);
		addMember(l,"tag",get_tag,set_tag,true);
		createTypeMetatable(l,constructor, typeof(UnityEngine.Component),typeof(UnityEngine.Object));
	}
}
