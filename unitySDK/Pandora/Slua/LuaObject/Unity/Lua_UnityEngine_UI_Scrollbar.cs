﻿using System;
using com.tencent.pandora;
using System.Collections.Generic;
public class Lua_UnityEngine_UI_Scrollbar : LuaObject {
	[com.tencent.pandora.MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
	static public int Rebuild(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.UI.Scrollbar.Rebuild");
			#endif
			UnityEngine.UI.Scrollbar self=(UnityEngine.UI.Scrollbar)checkSelf(l);
			UnityEngine.UI.CanvasUpdate a1;
			checkEnum(l,2,out a1);
			self.Rebuild(a1);
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
	[com.tencent.pandora.MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
	static public int LayoutComplete(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.UI.Scrollbar.LayoutComplete");
			#endif
			UnityEngine.UI.Scrollbar self=(UnityEngine.UI.Scrollbar)checkSelf(l);
			self.LayoutComplete();
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
	[com.tencent.pandora.MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
	static public int GraphicUpdateComplete(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.UI.Scrollbar.GraphicUpdateComplete");
			#endif
			UnityEngine.UI.Scrollbar self=(UnityEngine.UI.Scrollbar)checkSelf(l);
			self.GraphicUpdateComplete();
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
	[com.tencent.pandora.MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
	static public int OnBeginDrag(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.UI.Scrollbar.OnBeginDrag");
			#endif
			UnityEngine.UI.Scrollbar self=(UnityEngine.UI.Scrollbar)checkSelf(l);
			UnityEngine.EventSystems.PointerEventData a1;
			checkType(l,2,out a1);
			self.OnBeginDrag(a1);
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
	[com.tencent.pandora.MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
	static public int OnDrag(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.UI.Scrollbar.OnDrag");
			#endif
			UnityEngine.UI.Scrollbar self=(UnityEngine.UI.Scrollbar)checkSelf(l);
			UnityEngine.EventSystems.PointerEventData a1;
			checkType(l,2,out a1);
			self.OnDrag(a1);
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
	[com.tencent.pandora.MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
	static public int OnPointerDown(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.UI.Scrollbar.OnPointerDown");
			#endif
			UnityEngine.UI.Scrollbar self=(UnityEngine.UI.Scrollbar)checkSelf(l);
			UnityEngine.EventSystems.PointerEventData a1;
			checkType(l,2,out a1);
			self.OnPointerDown(a1);
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
	[com.tencent.pandora.MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
	static public int OnPointerUp(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.UI.Scrollbar.OnPointerUp");
			#endif
			UnityEngine.UI.Scrollbar self=(UnityEngine.UI.Scrollbar)checkSelf(l);
			UnityEngine.EventSystems.PointerEventData a1;
			checkType(l,2,out a1);
			self.OnPointerUp(a1);
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
	[com.tencent.pandora.MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
	static public int OnMove(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.UI.Scrollbar.OnMove");
			#endif
			UnityEngine.UI.Scrollbar self=(UnityEngine.UI.Scrollbar)checkSelf(l);
			UnityEngine.EventSystems.AxisEventData a1;
			checkType(l,2,out a1);
			self.OnMove(a1);
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
	[com.tencent.pandora.MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
	static public int FindSelectableOnLeft(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.UI.Scrollbar.FindSelectableOnLeft");
			#endif
			UnityEngine.UI.Scrollbar self=(UnityEngine.UI.Scrollbar)checkSelf(l);
			var ret=self.FindSelectableOnLeft();
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
	static public int FindSelectableOnRight(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.UI.Scrollbar.FindSelectableOnRight");
			#endif
			UnityEngine.UI.Scrollbar self=(UnityEngine.UI.Scrollbar)checkSelf(l);
			var ret=self.FindSelectableOnRight();
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
	static public int FindSelectableOnUp(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.UI.Scrollbar.FindSelectableOnUp");
			#endif
			UnityEngine.UI.Scrollbar self=(UnityEngine.UI.Scrollbar)checkSelf(l);
			var ret=self.FindSelectableOnUp();
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
	static public int FindSelectableOnDown(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.UI.Scrollbar.FindSelectableOnDown");
			#endif
			UnityEngine.UI.Scrollbar self=(UnityEngine.UI.Scrollbar)checkSelf(l);
			var ret=self.FindSelectableOnDown();
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
	static public int OnInitializePotentialDrag(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.UI.Scrollbar.OnInitializePotentialDrag");
			#endif
			UnityEngine.UI.Scrollbar self=(UnityEngine.UI.Scrollbar)checkSelf(l);
			UnityEngine.EventSystems.PointerEventData a1;
			checkType(l,2,out a1);
			self.OnInitializePotentialDrag(a1);
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
	[com.tencent.pandora.MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
	static public int SetDirection(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.UI.Scrollbar.SetDirection");
			#endif
			UnityEngine.UI.Scrollbar self=(UnityEngine.UI.Scrollbar)checkSelf(l);
			UnityEngine.UI.Scrollbar.Direction a1;
			checkEnum(l,2,out a1);
			System.Boolean a2;
			checkType(l,3,out a2);
			self.SetDirection(a1,a2);
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
	[com.tencent.pandora.MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
	static public int get_handleRect(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.UI.Scrollbar.handleRect");
			#endif
			UnityEngine.UI.Scrollbar self=(UnityEngine.UI.Scrollbar)checkSelf(l);
			pushValue(l,true);
			pushValue(l,self.handleRect);
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
	static public int set_handleRect(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.UI.Scrollbar.handleRect");
			#endif
			UnityEngine.UI.Scrollbar self=(UnityEngine.UI.Scrollbar)checkSelf(l);
			UnityEngine.RectTransform v;
			checkType(l,2,out v);
			self.handleRect=v;
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
	[com.tencent.pandora.MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
	static public int get_direction(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.UI.Scrollbar.direction");
			#endif
			UnityEngine.UI.Scrollbar self=(UnityEngine.UI.Scrollbar)checkSelf(l);
			pushValue(l,true);
			pushEnum(l,(int)self.direction);
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
	static public int set_direction(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.UI.Scrollbar.direction");
			#endif
			UnityEngine.UI.Scrollbar self=(UnityEngine.UI.Scrollbar)checkSelf(l);
			UnityEngine.UI.Scrollbar.Direction v;
			checkEnum(l,2,out v);
			self.direction=v;
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
	[com.tencent.pandora.MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
	static public int get_value(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.UI.Scrollbar.value");
			#endif
			UnityEngine.UI.Scrollbar self=(UnityEngine.UI.Scrollbar)checkSelf(l);
			pushValue(l,true);
			pushValue(l,self.value);
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
	static public int set_value(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.UI.Scrollbar.value");
			#endif
			UnityEngine.UI.Scrollbar self=(UnityEngine.UI.Scrollbar)checkSelf(l);
			float v;
			checkType(l,2,out v);
			self.value=v;
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
	[com.tencent.pandora.MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
	static public int get_size(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.UI.Scrollbar.size");
			#endif
			UnityEngine.UI.Scrollbar self=(UnityEngine.UI.Scrollbar)checkSelf(l);
			pushValue(l,true);
			pushValue(l,self.size);
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
	static public int set_size(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.UI.Scrollbar.size");
			#endif
			UnityEngine.UI.Scrollbar self=(UnityEngine.UI.Scrollbar)checkSelf(l);
			float v;
			checkType(l,2,out v);
			self.size=v;
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
	[com.tencent.pandora.MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
	static public int get_numberOfSteps(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.UI.Scrollbar.numberOfSteps");
			#endif
			UnityEngine.UI.Scrollbar self=(UnityEngine.UI.Scrollbar)checkSelf(l);
			pushValue(l,true);
			pushValue(l,self.numberOfSteps);
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
	static public int set_numberOfSteps(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.UI.Scrollbar.numberOfSteps");
			#endif
			UnityEngine.UI.Scrollbar self=(UnityEngine.UI.Scrollbar)checkSelf(l);
			int v;
			checkType(l,2,out v);
			self.numberOfSteps=v;
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
	[com.tencent.pandora.MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
	static public int get_onValueChanged(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.UI.Scrollbar.onValueChanged");
			#endif
			UnityEngine.UI.Scrollbar self=(UnityEngine.UI.Scrollbar)checkSelf(l);
			pushValue(l,true);
			pushValue(l,self.onValueChanged);
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
	static public int set_onValueChanged(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.UI.Scrollbar.onValueChanged");
			#endif
			UnityEngine.UI.Scrollbar self=(UnityEngine.UI.Scrollbar)checkSelf(l);
			UnityEngine.UI.Scrollbar.ScrollEvent v;
			checkType(l,2,out v);
			self.onValueChanged=v;
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
		getTypeTable(l,"UnityEngine.UI.Scrollbar");
		addMember(l,Rebuild);
		addMember(l,LayoutComplete);
		addMember(l,GraphicUpdateComplete);
		addMember(l,OnBeginDrag);
		addMember(l,OnDrag);
		addMember(l,OnPointerDown);
		addMember(l,OnPointerUp);
		addMember(l,OnMove);
		addMember(l,FindSelectableOnLeft);
		addMember(l,FindSelectableOnRight);
		addMember(l,FindSelectableOnUp);
		addMember(l,FindSelectableOnDown);
		addMember(l,OnInitializePotentialDrag);
		addMember(l,SetDirection);
		addMember(l,"handleRect",get_handleRect,set_handleRect,true);
		addMember(l,"direction",get_direction,set_direction,true);
		addMember(l,"value",get_value,set_value,true);
		addMember(l,"size",get_size,set_size,true);
		addMember(l,"numberOfSteps",get_numberOfSteps,set_numberOfSteps,true);
		addMember(l,"onValueChanged",get_onValueChanged,set_onValueChanged,true);
		createTypeMetatable(l,null, typeof(UnityEngine.UI.Scrollbar),typeof(UnityEngine.UI.Selectable));
	}
}
