﻿using System;
using com.tencent.pandora;
using System.Collections.Generic;
public class Lua_UnityEngine_Renderer : LuaObject {
	[com.tencent.pandora.MonoPInvokeCallbackAttribute(typeof(LuaCSFunction))]
	static public int constructor(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Renderer.constructor");
			#endif
			UnityEngine.Renderer o;
			o=new UnityEngine.Renderer();
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
	static public int HasPropertyBlock(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Renderer.HasPropertyBlock");
			#endif
			UnityEngine.Renderer self=(UnityEngine.Renderer)checkSelf(l);
			var ret=self.HasPropertyBlock();
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
	static public int SetPropertyBlock(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Renderer.SetPropertyBlock");
			#endif
			int argc = LuaDLL.pua_gettop(l);
			if(argc==2){
				UnityEngine.Renderer self=(UnityEngine.Renderer)checkSelf(l);
				UnityEngine.MaterialPropertyBlock a1;
				checkType(l,2,out a1);
				self.SetPropertyBlock(a1);
				pushValue(l,true);
				return 1;
			}
			else if(argc==3){
				UnityEngine.Renderer self=(UnityEngine.Renderer)checkSelf(l);
				UnityEngine.MaterialPropertyBlock a1;
				checkType(l,2,out a1);
				System.Int32 a2;
				checkType(l,3,out a2);
				self.SetPropertyBlock(a1,a2);
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
	static public int GetPropertyBlock(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Renderer.GetPropertyBlock");
			#endif
			int argc = LuaDLL.pua_gettop(l);
			if(argc==2){
				UnityEngine.Renderer self=(UnityEngine.Renderer)checkSelf(l);
				UnityEngine.MaterialPropertyBlock a1;
				checkType(l,2,out a1);
				self.GetPropertyBlock(a1);
				pushValue(l,true);
				return 1;
			}
			else if(argc==3){
				UnityEngine.Renderer self=(UnityEngine.Renderer)checkSelf(l);
				UnityEngine.MaterialPropertyBlock a1;
				checkType(l,2,out a1);
				System.Int32 a2;
				checkType(l,3,out a2);
				self.GetPropertyBlock(a1,a2);
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
	static public int GetMaterials(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Renderer.GetMaterials");
			#endif
			UnityEngine.Renderer self=(UnityEngine.Renderer)checkSelf(l);
			System.Collections.Generic.List<UnityEngine.Material> a1;
			checkType(l,2,out a1);
			self.GetMaterials(a1);
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
	static public int GetSharedMaterials(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Renderer.GetSharedMaterials");
			#endif
			UnityEngine.Renderer self=(UnityEngine.Renderer)checkSelf(l);
			System.Collections.Generic.List<UnityEngine.Material> a1;
			checkType(l,2,out a1);
			self.GetSharedMaterials(a1);
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
	static public int GetClosestReflectionProbes(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Renderer.GetClosestReflectionProbes");
			#endif
			UnityEngine.Renderer self=(UnityEngine.Renderer)checkSelf(l);
			System.Collections.Generic.List<UnityEngine.Rendering.ReflectionProbeBlendInfo> a1;
			checkType(l,2,out a1);
			self.GetClosestReflectionProbes(a1);
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
	static public int get_bounds(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Renderer.bounds");
			#endif
			UnityEngine.Renderer self=(UnityEngine.Renderer)checkSelf(l);
			pushValue(l,true);
			pushValue(l,self.bounds);
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
	static public int get_enabled(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Renderer.enabled");
			#endif
			UnityEngine.Renderer self=(UnityEngine.Renderer)checkSelf(l);
			pushValue(l,true);
			pushValue(l,self.enabled);
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
	static public int set_enabled(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Renderer.enabled");
			#endif
			UnityEngine.Renderer self=(UnityEngine.Renderer)checkSelf(l);
			bool v;
			checkType(l,2,out v);
			self.enabled=v;
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
	static public int get_isVisible(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Renderer.isVisible");
			#endif
			UnityEngine.Renderer self=(UnityEngine.Renderer)checkSelf(l);
			pushValue(l,true);
			pushValue(l,self.isVisible);
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
	static public int get_shadowCastingMode(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Renderer.shadowCastingMode");
			#endif
			UnityEngine.Renderer self=(UnityEngine.Renderer)checkSelf(l);
			pushValue(l,true);
			pushEnum(l,(int)self.shadowCastingMode);
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
	static public int set_shadowCastingMode(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Renderer.shadowCastingMode");
			#endif
			UnityEngine.Renderer self=(UnityEngine.Renderer)checkSelf(l);
			UnityEngine.Rendering.ShadowCastingMode v;
			checkEnum(l,2,out v);
			self.shadowCastingMode=v;
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
	static public int get_receiveShadows(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Renderer.receiveShadows");
			#endif
			UnityEngine.Renderer self=(UnityEngine.Renderer)checkSelf(l);
			pushValue(l,true);
			pushValue(l,self.receiveShadows);
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
	static public int set_receiveShadows(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Renderer.receiveShadows");
			#endif
			UnityEngine.Renderer self=(UnityEngine.Renderer)checkSelf(l);
			bool v;
			checkType(l,2,out v);
			self.receiveShadows=v;
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
	static public int get_motionVectorGenerationMode(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Renderer.motionVectorGenerationMode");
			#endif
			UnityEngine.Renderer self=(UnityEngine.Renderer)checkSelf(l);
			pushValue(l,true);
			pushEnum(l,(int)self.motionVectorGenerationMode);
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
	static public int set_motionVectorGenerationMode(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Renderer.motionVectorGenerationMode");
			#endif
			UnityEngine.Renderer self=(UnityEngine.Renderer)checkSelf(l);
			UnityEngine.MotionVectorGenerationMode v;
			checkEnum(l,2,out v);
			self.motionVectorGenerationMode=v;
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
	static public int get_lightProbeUsage(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Renderer.lightProbeUsage");
			#endif
			UnityEngine.Renderer self=(UnityEngine.Renderer)checkSelf(l);
			pushValue(l,true);
			pushEnum(l,(int)self.lightProbeUsage);
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
	static public int set_lightProbeUsage(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Renderer.lightProbeUsage");
			#endif
			UnityEngine.Renderer self=(UnityEngine.Renderer)checkSelf(l);
			UnityEngine.Rendering.LightProbeUsage v;
			checkEnum(l,2,out v);
			self.lightProbeUsage=v;
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
	static public int get_reflectionProbeUsage(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Renderer.reflectionProbeUsage");
			#endif
			UnityEngine.Renderer self=(UnityEngine.Renderer)checkSelf(l);
			pushValue(l,true);
			pushEnum(l,(int)self.reflectionProbeUsage);
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
	static public int set_reflectionProbeUsage(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Renderer.reflectionProbeUsage");
			#endif
			UnityEngine.Renderer self=(UnityEngine.Renderer)checkSelf(l);
			UnityEngine.Rendering.ReflectionProbeUsage v;
			checkEnum(l,2,out v);
			self.reflectionProbeUsage=v;
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
	static public int get_renderingLayerMask(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Renderer.renderingLayerMask");
			#endif
			UnityEngine.Renderer self=(UnityEngine.Renderer)checkSelf(l);
			pushValue(l,true);
			pushValue(l,self.renderingLayerMask);
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
	static public int set_renderingLayerMask(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Renderer.renderingLayerMask");
			#endif
			UnityEngine.Renderer self=(UnityEngine.Renderer)checkSelf(l);
			System.UInt32 v;
			checkType(l,2,out v);
			self.renderingLayerMask=v;
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
	static public int get_rendererPriority(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Renderer.rendererPriority");
			#endif
			UnityEngine.Renderer self=(UnityEngine.Renderer)checkSelf(l);
			pushValue(l,true);
			pushValue(l,self.rendererPriority);
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
	static public int set_rendererPriority(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Renderer.rendererPriority");
			#endif
			UnityEngine.Renderer self=(UnityEngine.Renderer)checkSelf(l);
			int v;
			checkType(l,2,out v);
			self.rendererPriority=v;
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
	static public int get_sortingLayerName(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Renderer.sortingLayerName");
			#endif
			UnityEngine.Renderer self=(UnityEngine.Renderer)checkSelf(l);
			pushValue(l,true);
			pushValue(l,self.sortingLayerName);
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
	static public int set_sortingLayerName(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Renderer.sortingLayerName");
			#endif
			UnityEngine.Renderer self=(UnityEngine.Renderer)checkSelf(l);
			string v;
			checkType(l,2,out v);
			self.sortingLayerName=v;
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
	static public int get_sortingLayerID(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Renderer.sortingLayerID");
			#endif
			UnityEngine.Renderer self=(UnityEngine.Renderer)checkSelf(l);
			pushValue(l,true);
			pushValue(l,self.sortingLayerID);
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
	static public int set_sortingLayerID(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Renderer.sortingLayerID");
			#endif
			UnityEngine.Renderer self=(UnityEngine.Renderer)checkSelf(l);
			int v;
			checkType(l,2,out v);
			self.sortingLayerID=v;
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
	static public int get_sortingOrder(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Renderer.sortingOrder");
			#endif
			UnityEngine.Renderer self=(UnityEngine.Renderer)checkSelf(l);
			pushValue(l,true);
			pushValue(l,self.sortingOrder);
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
	static public int set_sortingOrder(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Renderer.sortingOrder");
			#endif
			UnityEngine.Renderer self=(UnityEngine.Renderer)checkSelf(l);
			int v;
			checkType(l,2,out v);
			self.sortingOrder=v;
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
	static public int get_allowOcclusionWhenDynamic(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Renderer.allowOcclusionWhenDynamic");
			#endif
			UnityEngine.Renderer self=(UnityEngine.Renderer)checkSelf(l);
			pushValue(l,true);
			pushValue(l,self.allowOcclusionWhenDynamic);
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
	static public int set_allowOcclusionWhenDynamic(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Renderer.allowOcclusionWhenDynamic");
			#endif
			UnityEngine.Renderer self=(UnityEngine.Renderer)checkSelf(l);
			bool v;
			checkType(l,2,out v);
			self.allowOcclusionWhenDynamic=v;
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
	static public int get_isPartOfStaticBatch(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Renderer.isPartOfStaticBatch");
			#endif
			UnityEngine.Renderer self=(UnityEngine.Renderer)checkSelf(l);
			pushValue(l,true);
			pushValue(l,self.isPartOfStaticBatch);
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
	static public int get_worldToLocalMatrix(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Renderer.worldToLocalMatrix");
			#endif
			UnityEngine.Renderer self=(UnityEngine.Renderer)checkSelf(l);
			pushValue(l,true);
			pushValue(l,self.worldToLocalMatrix);
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
	static public int get_localToWorldMatrix(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Renderer.localToWorldMatrix");
			#endif
			UnityEngine.Renderer self=(UnityEngine.Renderer)checkSelf(l);
			pushValue(l,true);
			pushValue(l,self.localToWorldMatrix);
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
	static public int get_lightProbeProxyVolumeOverride(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Renderer.lightProbeProxyVolumeOverride");
			#endif
			UnityEngine.Renderer self=(UnityEngine.Renderer)checkSelf(l);
			pushValue(l,true);
			pushValue(l,self.lightProbeProxyVolumeOverride);
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
	static public int set_lightProbeProxyVolumeOverride(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Renderer.lightProbeProxyVolumeOverride");
			#endif
			UnityEngine.Renderer self=(UnityEngine.Renderer)checkSelf(l);
			UnityEngine.GameObject v;
			checkType(l,2,out v);
			self.lightProbeProxyVolumeOverride=v;
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
	static public int get_probeAnchor(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Renderer.probeAnchor");
			#endif
			UnityEngine.Renderer self=(UnityEngine.Renderer)checkSelf(l);
			pushValue(l,true);
			pushValue(l,self.probeAnchor);
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
	static public int set_probeAnchor(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Renderer.probeAnchor");
			#endif
			UnityEngine.Renderer self=(UnityEngine.Renderer)checkSelf(l);
			UnityEngine.Transform v;
			checkType(l,2,out v);
			self.probeAnchor=v;
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
	static public int get_lightmapIndex(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Renderer.lightmapIndex");
			#endif
			UnityEngine.Renderer self=(UnityEngine.Renderer)checkSelf(l);
			pushValue(l,true);
			pushValue(l,self.lightmapIndex);
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
	static public int set_lightmapIndex(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Renderer.lightmapIndex");
			#endif
			UnityEngine.Renderer self=(UnityEngine.Renderer)checkSelf(l);
			int v;
			checkType(l,2,out v);
			self.lightmapIndex=v;
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
	static public int get_realtimeLightmapIndex(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Renderer.realtimeLightmapIndex");
			#endif
			UnityEngine.Renderer self=(UnityEngine.Renderer)checkSelf(l);
			pushValue(l,true);
			pushValue(l,self.realtimeLightmapIndex);
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
	static public int set_realtimeLightmapIndex(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Renderer.realtimeLightmapIndex");
			#endif
			UnityEngine.Renderer self=(UnityEngine.Renderer)checkSelf(l);
			int v;
			checkType(l,2,out v);
			self.realtimeLightmapIndex=v;
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
	static public int get_lightmapScaleOffset(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Renderer.lightmapScaleOffset");
			#endif
			UnityEngine.Renderer self=(UnityEngine.Renderer)checkSelf(l);
			pushValue(l,true);
			pushValue(l,self.lightmapScaleOffset);
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
	static public int set_lightmapScaleOffset(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Renderer.lightmapScaleOffset");
			#endif
			UnityEngine.Renderer self=(UnityEngine.Renderer)checkSelf(l);
			UnityEngine.Vector4 v;
			checkType(l,2,out v);
			self.lightmapScaleOffset=v;
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
	static public int get_realtimeLightmapScaleOffset(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Renderer.realtimeLightmapScaleOffset");
			#endif
			UnityEngine.Renderer self=(UnityEngine.Renderer)checkSelf(l);
			pushValue(l,true);
			pushValue(l,self.realtimeLightmapScaleOffset);
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
	static public int set_realtimeLightmapScaleOffset(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Renderer.realtimeLightmapScaleOffset");
			#endif
			UnityEngine.Renderer self=(UnityEngine.Renderer)checkSelf(l);
			UnityEngine.Vector4 v;
			checkType(l,2,out v);
			self.realtimeLightmapScaleOffset=v;
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
	static public int get_materials(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Renderer.materials");
			#endif
			UnityEngine.Renderer self=(UnityEngine.Renderer)checkSelf(l);
			pushValue(l,true);
			pushValue(l,self.materials);
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
	static public int set_materials(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Renderer.materials");
			#endif
			UnityEngine.Renderer self=(UnityEngine.Renderer)checkSelf(l);
			UnityEngine.Material[] v;
			checkArray(l,2,out v);
			self.materials=v;
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
	static public int get_material(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Renderer.material");
			#endif
			UnityEngine.Renderer self=(UnityEngine.Renderer)checkSelf(l);
			pushValue(l,true);
			pushValue(l,self.material);
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
	static public int set_material(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Renderer.material");
			#endif
			UnityEngine.Renderer self=(UnityEngine.Renderer)checkSelf(l);
			UnityEngine.Material v;
			checkType(l,2,out v);
			self.material=v;
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
	static public int get_sharedMaterial(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Renderer.sharedMaterial");
			#endif
			UnityEngine.Renderer self=(UnityEngine.Renderer)checkSelf(l);
			pushValue(l,true);
			pushValue(l,self.sharedMaterial);
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
	static public int set_sharedMaterial(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Renderer.sharedMaterial");
			#endif
			UnityEngine.Renderer self=(UnityEngine.Renderer)checkSelf(l);
			UnityEngine.Material v;
			checkType(l,2,out v);
			self.sharedMaterial=v;
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
	static public int get_sharedMaterials(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Renderer.sharedMaterials");
			#endif
			UnityEngine.Renderer self=(UnityEngine.Renderer)checkSelf(l);
			pushValue(l,true);
			pushValue(l,self.sharedMaterials);
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
	static public int set_sharedMaterials(IntPtr l) {
		try {
			#if PANDORA_PROFILE
			beginSample("[C#]UnityEngine.Renderer.sharedMaterials");
			#endif
			UnityEngine.Renderer self=(UnityEngine.Renderer)checkSelf(l);
			UnityEngine.Material[] v;
			checkArray(l,2,out v);
			self.sharedMaterials=v;
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
		getTypeTable(l,"UnityEngine.Renderer");
		addMember(l,HasPropertyBlock);
		addMember(l,SetPropertyBlock);
		addMember(l,GetPropertyBlock);
		addMember(l,GetMaterials);
		addMember(l,GetSharedMaterials);
		addMember(l,GetClosestReflectionProbes);
		addMember(l,"bounds",get_bounds,null,true);
		addMember(l,"enabled",get_enabled,set_enabled,true);
		addMember(l,"isVisible",get_isVisible,null,true);
		addMember(l,"shadowCastingMode",get_shadowCastingMode,set_shadowCastingMode,true);
		addMember(l,"receiveShadows",get_receiveShadows,set_receiveShadows,true);
		addMember(l,"motionVectorGenerationMode",get_motionVectorGenerationMode,set_motionVectorGenerationMode,true);
		addMember(l,"lightProbeUsage",get_lightProbeUsage,set_lightProbeUsage,true);
		addMember(l,"reflectionProbeUsage",get_reflectionProbeUsage,set_reflectionProbeUsage,true);
		addMember(l,"renderingLayerMask",get_renderingLayerMask,set_renderingLayerMask,true);
		addMember(l,"rendererPriority",get_rendererPriority,set_rendererPriority,true);
		addMember(l,"sortingLayerName",get_sortingLayerName,set_sortingLayerName,true);
		addMember(l,"sortingLayerID",get_sortingLayerID,set_sortingLayerID,true);
		addMember(l,"sortingOrder",get_sortingOrder,set_sortingOrder,true);
		addMember(l,"allowOcclusionWhenDynamic",get_allowOcclusionWhenDynamic,set_allowOcclusionWhenDynamic,true);
		addMember(l,"isPartOfStaticBatch",get_isPartOfStaticBatch,null,true);
		addMember(l,"worldToLocalMatrix",get_worldToLocalMatrix,null,true);
		addMember(l,"localToWorldMatrix",get_localToWorldMatrix,null,true);
		addMember(l,"lightProbeProxyVolumeOverride",get_lightProbeProxyVolumeOverride,set_lightProbeProxyVolumeOverride,true);
		addMember(l,"probeAnchor",get_probeAnchor,set_probeAnchor,true);
		addMember(l,"lightmapIndex",get_lightmapIndex,set_lightmapIndex,true);
		addMember(l,"realtimeLightmapIndex",get_realtimeLightmapIndex,set_realtimeLightmapIndex,true);
		addMember(l,"lightmapScaleOffset",get_lightmapScaleOffset,set_lightmapScaleOffset,true);
		addMember(l,"realtimeLightmapScaleOffset",get_realtimeLightmapScaleOffset,set_realtimeLightmapScaleOffset,true);
		addMember(l,"materials",get_materials,set_materials,true);
		addMember(l,"material",get_material,set_material,true);
		addMember(l,"sharedMaterial",get_sharedMaterial,set_sharedMaterial,true);
		addMember(l,"sharedMaterials",get_sharedMaterials,set_sharedMaterials,true);
		createTypeMetatable(l,constructor, typeof(UnityEngine.Renderer),typeof(UnityEngine.Component));
	}
}
