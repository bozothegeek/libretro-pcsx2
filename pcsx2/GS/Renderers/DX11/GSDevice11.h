/*
 *	Copyright (C) 2007-2009 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

#include <unordered_map>

#include "Pcsx2Types.h"

#include "GSTexture11.h"
#include "../../GSVector.h"
#include "../Common/GSDevice.h"

#include "libretro_d3d.h"

extern retro_environment_t environ_cb;

struct GSVertexShader11
{
	CComPtr<ID3D11VertexShader> vs;
	CComPtr<ID3D11InputLayout> il;
};

class GSDevice11 final : public GSDevice
{
public:
	#pragma pack(push, 1)

	struct alignas(32) VSConstantBuffer
	{
		GSVector4 VertexScale;
		GSVector4 VertexOffset;
		GSVector4 Texture_Scale_Offset;
		GSVector2i MaxDepth;
		GSVector2i pad_vscb;

		VSConstantBuffer()
		{
			VertexScale          = GSVector4::zero();
			VertexOffset         = GSVector4::zero();
			Texture_Scale_Offset = GSVector4::zero();
			MaxDepth             = GSVector2i(0);
			pad_vscb             = GSVector2i(0);
		}

		GS_FORCEINLINE bool Update(const VSConstantBuffer* cb)
		{
			GSVector4i* a = (GSVector4i*)this;
			GSVector4i* b = (GSVector4i*)cb;

			if(!((a[0] == b[0]) & (a[1] == b[1]) & (a[2] == b[2]) & (a[3] == b[3])).alltrue())
			{
				a[0] = b[0];
				a[1] = b[1];
				a[2] = b[2];
				a[3] = b[3];

				return true;
			}

			return false;
		}
	};

	struct VSSelector
	{
		union
		{
			struct
			{
				u32 tme:1;
				u32 fst:1;

				u32 _free:30;
			};

			u32 key;
		};

		operator u32() const {return key;}

		VSSelector() : key(0) {}
		VSSelector(u32 k) : key(k) {}
	};

	struct alignas(32) PSConstantBuffer
	{
		GSVector4 FogColor_AREF;
		GSVector4 HalfTexel;
		GSVector4 WH;
		GSVector4 MinMax;
		GSVector4 MinF_TA;
		GSVector4i MskFix;
		GSVector4i ChannelShuffle;
		GSVector4i FbMask;

		GSVector4 TC_OffsetHack;
		GSVector4 Af_MaxDepth;
		GSVector4 DitherMatrix[4];

		PSConstantBuffer()
		{
			FogColor_AREF = GSVector4::zero();
			HalfTexel = GSVector4::zero();
			WH = GSVector4::zero();
			MinMax = GSVector4::zero();
			MinF_TA = GSVector4::zero();
			MskFix = GSVector4i::zero();
			ChannelShuffle = GSVector4i::zero();
			FbMask = GSVector4i::zero();
			Af_MaxDepth = GSVector4::zero();

			DitherMatrix[0] = GSVector4::zero();
			DitherMatrix[1] = GSVector4::zero();
			DitherMatrix[2] = GSVector4::zero();
			DitherMatrix[3] = GSVector4::zero();
		}

		GS_FORCEINLINE bool Update(const PSConstantBuffer* cb)
		{
			GSVector4i* a = (GSVector4i*)this;
			GSVector4i* b = (GSVector4i*)cb;

			if(!((a[0] == b[0]) /*& (a[1] == b1)*/ & (a[2] == b[2]) & (a[3] == b[3]) & (a[4] == b[4]) & (a[5] == b[5]) &
				(a[6] == b[6]) & (a[7] == b[7]) & (a[9] == b[9]) & // if WH matches HalfTexel does too
				(a[10] == b[10]) & (a[11] == b[11]) & (a[12] == b[12]) & (a[13] == b[13])).alltrue())
			{
				a[0] = b[0];
				a[1] = b[1];
				a[2] = b[2];
				a[3] = b[3];
				a[4] = b[4];
				a[5] = b[5];
				a[6] = b[6];
				a[7] = b[7];
				a[9] = b[9];

				a[10] = b[10];
				a[11] = b[11];
				a[12] = b[12];
				a[13] = b[13];

				return true;
			}

			return false;
		}
	};

	struct alignas(32) GSConstantBuffer
	{
		GSVector2 PointSize;

		GSConstantBuffer()
		{
			PointSize = GSVector2(0);
		}

		GS_FORCEINLINE bool Update(const GSConstantBuffer* cb)
		{
			return true;
		}
	};

	struct GSSelector
	{
		union
		{
			struct
			{
				u32 iip:1;
				u32 prim:2;
				u32 point:1;
				u32 line:1;
				u32 cpu_sprite:1;

				u32 _free:26;
			};

			u32 key;
		};

		operator u32() {return key;}

		GSSelector() : key(0) {}
		GSSelector(u32 k) : key(k) {}
	};

	struct PSSelector
	{
		union
		{
			struct
			{
				// *** Word 1
				// Format
				u32 fmt:4;
				u32 dfmt:2;
				u32 depth_fmt:2;
				// Alpha extension/Correction
				u32 aem:1;
				u32 fba:1;
				// Fog
				u32 fog:1;
				// Pixel test
				u32 atst:3;
				// Color sampling
				u32 fst:1;
				u32 tfx:3;
				u32 tcc:1;
				u32 wms:2;
				u32 wmt:2;
				u32 ltf:1;
				// Shuffle and fbmask effect
				u32 shuffle:1;
				u32 read_ba:1;
				u32 fbmask:1;

				// Blend and Colclip
				u32 hdr:1;
				u32 blend_a:2;
				u32 blend_b:2; // bit30/31
				u32 blend_c:2; // bit0
				u32 blend_d:2;
				u32 clr1:1;

				u32 colclip:1;
            u32 pabe:1;

				// Others ways to fetch the texture
				u32 channel:3;

				// Dithering
				u32 dither:2;

				// Depth clamp
				u32 zclamp:1;

				// Hack
				u32 tcoffsethack:1;
				u32 urban_chaos_hle:1;
				u32 tales_of_abyss_hle:1;
				u32 point_sampler:1;
				u32 invalid_tex0:1; // Lupin the 3rd

				u32 _free:14;
			};

			u64 key;
		};

		operator u64() {return key;}

		PSSelector() : key(0) {}
	};

	struct PSSamplerSelector
	{
		union
		{
			struct
			{
				u32 tau:1;
				u32 tav:1;
				u32 ltf:1;
			};

			u32 key;
		};

		operator u32() {return key & 0x7;}

		PSSamplerSelector() : key(0) {}
	};

	struct OMDepthStencilSelector
	{
		union
		{
			struct
			{
				u32 ztst:2;
				u32 zwe:1;
				u32 date:1;
				u32 fba:1;
				u32 date_one:1;
			};

			u32 key;
		};

		operator u32() {return key & 0x3f;}

		OMDepthStencilSelector() : key(0) {}
	};

	struct OMBlendSelector
	{
		union
		{
			struct
			{
				/* Color mask */
				u32 wr:1;
				u32 wg:1;
				u32 wb:1;
				u32 wa:1;
				/* Alpha blending */
				u32 blend_index:7;
				u32 abe:1;
				u32 accu_blend:1;
				u32 blend_mix : 1;
			};

			struct
			{
				/* Color mask */
				u32 wrgba:4;
			};

			u32 key;
		};

		operator u32() {return key & 0x3fff;}

		OMBlendSelector() : key(0) {}
	};

	#pragma pack(pop)

	class ShaderMacro
	{
		struct mcstr
		{
			const char* name, * def;
			mcstr(const char* n, const char* d) : name(n), def(d) {}
		};

		struct mstring
		{
			std::string name, def;
			mstring(const char* n, std::string d) : name(n), def(d) {}
		};

		std::vector<mstring> mlist;
		std::vector<mcstr> mout;

	public:
		ShaderMacro(std::string& smodel);
		void AddMacro(const char* n, int d);
		D3D_SHADER_MACRO* GetPtr(void);
	};

private:
	int m_upscale_multiplier;
	int m_aniso_filter;
	int m_mipmap;
	int m_d3d_texsize;

	GSTexture* CreateSurface(int type, int w, int h, int format);
	GSTexture* FetchSurface(int type, int w, int h, int format);

	void DoMerge(GSTexture* sTex[3], GSVector4* sRect, GSTexture* dTex, GSVector4* dRect, const GSRegPMODE& PMODE, const GSRegEXTBUF& EXTBUF, const GSVector4& c) final;
	void DoInterlace(GSTexture* sTex, GSTexture* dTex, int shader, bool linear, float yoffset = 0) final;
	void DoFXAA(GSTexture* sTex, GSTexture* dTex) final;
	void BeforeDraw();
	void AfterDraw();
	
	u16 ConvertBlendEnum(u16 generic) final;

	CComPtr<IDXGIFactory2> m_factory;
	CComPtr<ID3D11Device> m_dev;
	CComPtr<ID3D11DeviceContext> m_ctx;
	CComPtr<ID3D11Buffer> m_vb;
	CComPtr<ID3D11Buffer> m_vb_old;
	CComPtr<ID3D11Buffer> m_ib;
	CComPtr<ID3D11Buffer> m_ib_old;

	struct
	{
		ID3D11Buffer* vb;
		size_t vb_stride;
		ID3D11Buffer* ib;
		ID3D11InputLayout* layout;
		D3D11_PRIMITIVE_TOPOLOGY topology;
		ID3D11VertexShader* vs;
		ID3D11Buffer* vs_cb;
		ID3D11GeometryShader* gs;
		ID3D11Buffer* gs_cb;
		std::array<ID3D11ShaderResourceView*, 16> ps_sr_views;
		std::array<GSTexture11*, 16> ps_sr_texture;
		ID3D11PixelShader* ps;
		ID3D11Buffer* ps_cb;
		ID3D11SamplerState* ps_ss[3];
		GSVector2i viewport;
		GSVector4i scissor;
		ID3D11DepthStencilState* dss;
		u8 sref;
		ID3D11BlendState* bs;
		float bf;
		ID3D11RenderTargetView* rt_view;
		GSTexture11* rt_texture;
		GSTexture11* rt_ds;
		ID3D11DepthStencilView* dsv;
		uint16_t ps_sr_bitfield;
	} m_state;

	CComPtr<ID3D11RasterizerState> m_rs;

	struct
	{
		CComPtr<ID3D11InputLayout> il;
		CComPtr<ID3D11VertexShader> vs;
		CComPtr<ID3D11PixelShader> ps[ShaderConvert_Count];
		CComPtr<ID3D11SamplerState> ln;
		CComPtr<ID3D11SamplerState> pt;
		CComPtr<ID3D11DepthStencilState> dss;
		CComPtr<ID3D11DepthStencilState> dss_write;
		CComPtr<ID3D11BlendState> bs;
	} m_convert;

	struct
	{
		CComPtr<ID3D11PixelShader> ps[2];
		CComPtr<ID3D11Buffer> cb;
		CComPtr<ID3D11BlendState> bs;
	} m_merge;

	struct
	{
		CComPtr<ID3D11PixelShader> ps[4];
		CComPtr<ID3D11Buffer> cb;
	} m_interlace;

	struct 
	{
		CComPtr<ID3D11PixelShader> ps;
		CComPtr<ID3D11Buffer> cb;
	} m_fxaa;

	struct
	{
		CComPtr<ID3D11DepthStencilState> dss;
		CComPtr<ID3D11BlendState> bs;
	} m_date;

	// Shaders...

	std::unordered_map<u32, GSVertexShader11> m_vs;
	CComPtr<ID3D11Buffer> m_vs_cb;
	std::unordered_map<u32, CComPtr<ID3D11GeometryShader>> m_gs;
	CComPtr<ID3D11Buffer> m_gs_cb;
	std::unordered_map<u64, CComPtr<ID3D11PixelShader>> m_ps;
	CComPtr<ID3D11Buffer> m_ps_cb;
	std::unordered_map<u32, CComPtr<ID3D11SamplerState>> m_ps_ss;
	CComPtr<ID3D11SamplerState> m_palette_ss;
	std::unordered_map<u32, CComPtr<ID3D11DepthStencilState>> m_om_dss;
	std::unordered_map<u32, CComPtr<ID3D11BlendState>> m_om_bs;

	VSConstantBuffer m_vs_cb_cache;
	GSConstantBuffer m_gs_cb_cache;
	PSConstantBuffer m_ps_cb_cache;
protected:
	struct {D3D_FEATURE_LEVEL level; std::string model, vs, gs, ps, cs;} m_shader;
public:
	GSDevice11();
	virtual ~GSDevice11() {}

	bool SetFeatureLevel(D3D_FEATURE_LEVEL level, bool compat_mode);
	void GetFeatureLevel(D3D_FEATURE_LEVEL& level) const { level = m_shader.level; }

	bool Create();
	bool Reset(int w, int h);
	void Flip();

	void DrawPrimitive();
	void DrawIndexedPrimitive();
	void DrawIndexedPrimitive(int offset, int count);

	void ClearRenderTarget(GSTexture* t, const GSVector4& c) final;
	void ClearRenderTarget(GSTexture* t, u32 c) final;
	void ClearDepth(GSTexture* t) final;
	void ClearStencil(GSTexture* t, u8 c) final;

	GSTexture* CopyOffscreen(GSTexture* src, const GSVector4& sRect, int w, int h, int format = 0, int ps_shader = 0) final;

	void CloneTexture(GSTexture* src, GSTexture** dest);

	void CopyRect(GSTexture* sTex, GSTexture* dTex, const GSVector4i& r);

	void StretchRect(GSTexture* sTex, const GSVector4& sRect, GSTexture* dTex, const GSVector4& dRect, int shader = 0, bool linear = true) final;
	void StretchRect(GSTexture* sTex, const GSVector4& sRect, GSTexture* dTex, const GSVector4& dRect, ID3D11PixelShader* ps, ID3D11Buffer* ps_cb, bool linear = true);
	void StretchRect(GSTexture* sTex, const GSVector4& sRect, GSTexture* dTex, const GSVector4& dRect, bool red, bool green, bool blue, bool alpha);
	void StretchRect(GSTexture* sTex, const GSVector4& sRect, GSTexture* dTex, const GSVector4& dRect, ID3D11PixelShader* ps, ID3D11Buffer* ps_cb, ID3D11BlendState* bs, bool linear = true);

	void SetupDATE(GSTexture* rt, GSTexture* ds, const GSVertexPT1* vertices, bool datm);

	void IASetVertexBuffer(const void* vertex, size_t stride, size_t count);
	bool IAMapVertexBuffer(void** vertex, size_t stride, size_t count);
	void IAUnmapVertexBuffer();
	void IASetVertexBuffer(ID3D11Buffer* vb, size_t stride);
	void IASetIndexBuffer(const void* index, size_t count);
	void IASetIndexBuffer(ID3D11Buffer* ib);
	void IASetInputLayout(ID3D11InputLayout* layout);
	void IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY topology);

	void VSSetShader(ID3D11VertexShader* vs, ID3D11Buffer* vs_cb);
	void GSSetShader(ID3D11GeometryShader* gs, ID3D11Buffer* gs_cb = NULL);

	void PSSetShaderResources(GSTexture* sr0, GSTexture* sr1);
	void PSSetShaderResource(int i, GSTexture* sr);
	void PSSetShaderResourceView(int i, ID3D11ShaderResourceView* srv, GSTexture* sr);
	void PSSetShader(ID3D11PixelShader* ps, ID3D11Buffer* ps_cb);
	void PSUpdateShaderState();
	void PSSetSamplerState(ID3D11SamplerState* ss0, ID3D11SamplerState* ss1);

	void OMSetDepthStencilState(ID3D11DepthStencilState* dss, u8 sref);
	void OMSetBlendState(ID3D11BlendState* bs, float bf);
	void OMSetRenderTargets(GSTexture* rt, GSTexture* ds, const GSVector4i* scissor = NULL);

	bool CreateTextureFX();
	void SetupVS(VSSelector sel, const VSConstantBuffer* cb);
	void SetupGS(GSSelector sel, const GSConstantBuffer* cb);
	void SetupPS(PSSelector sel, const PSConstantBuffer* cb, PSSamplerSelector ssel);
	void SetupOM(OMDepthStencilSelector dssel, OMBlendSelector bsel, u8 afix);

	ID3D11Device* operator->() {return m_dev;}
	operator ID3D11Device*() {return m_dev;}
	operator ID3D11DeviceContext*() {return m_ctx;}

	void CreateShader(const std::vector<char>& source, const char* fn, ID3DInclude* include, const char* entry, D3D_SHADER_MACRO* macro, ID3D11VertexShader** vs, D3D11_INPUT_ELEMENT_DESC* layout, int count, ID3D11InputLayout** il);
	void CreateShader(const std::vector<char> &source, const char* fn, ID3DInclude* include, const char* entry, D3D_SHADER_MACRO* macro, ID3D11GeometryShader** gs);
	void CreateShader(const std::vector<char> &source, const char* fn, ID3DInclude *include, const char* entry, D3D_SHADER_MACRO* macro, ID3D11PixelShader** ps);

	void CompileShader(const std::vector<char> &source, const char* fn, ID3DInclude *include, const char* entry, D3D_SHADER_MACRO* macro, ID3DBlob** shader, std::string shader_model);
};

