#pragma once
#include "Common/d3dApp.h"
#include "Common/MathHelper.h"
#include "Common/UploadBuffer.h"
#include "Common/Camera.h"
#include "FrameResource.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <wrl.h>
using Microsoft::WRL::ComPtr;
using namespace DirectX;
struct RenderItem {
    XMFLOAT4X4 World = MathHelper::Identity4x4();
    XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();
    MeshGeometry* Geo = nullptr;
    D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    UINT IndexCount = 0;
    UINT StartIndexLocation = 0;
    int BaseVertexLocation = 0;
    int ObjCBIndex = -1;
    UINT MaterialIndex = 0;
    int DiffuseSrvHeapIndex = 0;
    bool IsAnimated = false;
};

struct TextureAsset {
    std::string Name;
    Microsoft::WRL::ComPtr<ID3D12Resource> Resource = nullptr;
    Microsoft::WRL::ComPtr<ID3D12Resource> UploadHeap = nullptr;
};

class TowerGameApp : public D3DApp {
public:
    TowerGameApp(HINSTANCE hInstance);
    virtual ~TowerGameApp() override;
    virtual bool Initialize() override;
    virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
private:
    POINT mLastMousePos;
    float mPitch = 0.20f;
    float mYaw = 0.0f;
    XMFLOAT3 mPlayerPosition = { 0.0f, 10.0f, -30.0f };
    float mCameraDistance = 20.0f;
    float mMinCameraDistance = 11.0f;
    float mMaxCameraDistance = 28.0f;
    float mCameraLookOffset = 7.0f;
    bool mFreeFlyMode = true;
    bool mToggleViewPressed = false;
    bool mHasMousePosition = false;
    float mFlyPitch = 0.05f;
    float mFlyYaw = 0.0f;
    XMFLOAT3 mFlyCameraPosition = { 0.0f, 85.0f, -110.0f };
    HWND mControlsOverlay = nullptr;
    HFONT mControlsFont = nullptr;
    HBRUSH mControlsBrush = nullptr;
    // Pseudo-Physics
    float mVerticalVelocity = 0.0f;
    float eyeLevel = 10.0f;
    float carHeight = 1000.0f + 12.0f; // add 12.0 so car is on "floor"
    float mSummitPedestalTop = 1088.0f;
    bool mIsJumping = false;
    bool mHasWon = false;   
    virtual void OnMouseDown(WPARAM btnState, int x, int y) override;
    virtual void OnMouseUp(WPARAM btnState, int x, int y) override;
    virtual void OnMouseMove(WPARAM btnState, int x, int y) override;
    virtual void OnResize() override;
    virtual void Update(const GameTimer& gt) override;
    virtual void Draw(const GameTimer& gt) override;
    void BuildTowerGeometry();
    void BuildCustomGeometry();
    void BuildCarGeometry();
    void BuildCharacterGeometry();
    void BuildTextures();
    void BuildDescriptorHeaps();
    void BuildRenderItems();
    void BuildShadersAndInputLayout();
    void BuildPSOs();
    void BuildFrameResources();
    void BuildRootSignature();
    void CreateControlsOverlay();
    void UpdateControlsOverlay();
    void UpdateCharacterTransform();
    void UpdateChaseCamera();
    void UpdateFlyCamera();
    std::vector<std::unique_ptr<FrameResource>> mFrameResources;
    FrameResource* mCurrFrameResource = nullptr;
    int mCurrFrameResourceIndex = 0;
    ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
    ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;
    ComPtr<ID3D12PipelineState> mOpaquePSO = nullptr;
    ComPtr<ID3D12PipelineState> mTransparentPSO = nullptr;   
    ComPtr<ID3D12PipelineState> mStencilMaskPSO = nullptr;
    ComPtr<ID3D12PipelineState> mStencilPortalPSO = nullptr;
    ComPtr<ID3DBlob> mvsByteCode = nullptr;
    ComPtr<ID3DBlob> mpsByteCode = nullptr;
    ComPtr<ID3DBlob> mgsByteCode = nullptr;            
    ComPtr<ID3D12PipelineState> mSparkPSO = nullptr;   
    ComPtr<ID3D12PipelineState> mTorchPSO = nullptr;
    ComPtr<ID3DBlob> mgsTorchByteCode = nullptr;
    ComPtr<ID3DBlob> mvsTorchByteCode = nullptr;
    ComPtr<ID3DBlob> mpsTorchByteCode = nullptr;
    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
    std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
    std::unordered_map<std::string, std::unique_ptr<TextureAsset>> mTextures;
    std::vector<std::unique_ptr<RenderItem>> mAllRitems;
    std::vector<RenderItem*> mOpaqueRitems;
    std::vector<RenderItem*> mPlatformRitems;
    std::vector<RenderItem*> mSparkRitems;
    std::vector<RenderItem*> mTorchRitems;
    std::vector<RenderItem*> mStencilMaskRitems;
    std::vector<RenderItem*> mPortalRitems;
    RenderItem* mOrbRitem = nullptr;                         
    RenderItem* mCarRitem = nullptr;
    RenderItem* mCharacterRitem = nullptr;
    RenderItem* mPedestalRitem = nullptr;
    Camera mCamera;
};
