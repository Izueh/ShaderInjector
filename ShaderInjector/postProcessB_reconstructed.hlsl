// HLSL function stub generated

//SamplerComparisonState View_SharedBilinearClampedSampler : register(s0); // can't disambiguate

SamplerState View_SharedBilinearClampedSampler : register(s0); // can't disambiguate

Texture3D<float4> View_SpatiotemporalBlueNoiseVolumeTexture : register(t0);
Texture2D<float4> ColorTexture : register(t1);
Texture2D<float4> GlareTexture : register(t2);
Texture2D<float4> CompositeSDRTexture : register(t3);
Texture2D<float4> CompositeSDRBackgroundTexture : register(t4);
Texture2D<float4> CompositeSDRForegroundTexture : register(t5);
Texture3D<float4> BT709PQToBT2020PQLUT : register(t6);
Texture3D<float4> BT2020PQTosRGBLUT : register(t7);

cbuffer _Globals : register(b0) 
{
	float4 PostprocessInput0Size : packoffset(c0.x);
	float4 PostprocessInput1Size : packoffset(c1.x);
	float4 PostprocessInput2Size : packoffset(c2.x);
	float4 PostprocessInput3Size : packoffset(c3.x);
	float4 PostprocessInput4Size : packoffset(c4.x);
	float4 PostprocessInput5Size : packoffset(c5.x);
	float4 PostprocessInput6Size : packoffset(c6.x);
	float4 PostprocessInput7Size : packoffset(c7.x);
	float4 PostprocessInput8Size : packoffset(c8.x);
	float4 PostprocessInput9Size : packoffset(c9.x);
	float4 PostprocessInput10Size : packoffset(c10.x);
	float4 PostprocessInput0MinMax : packoffset(c11.x);
	float4 PostprocessInput1MinMax : packoffset(c12.x);
	float4 PostprocessInput2MinMax : packoffset(c13.x);
	float4 PostprocessInput3MinMax : packoffset(c14.x);
	float4 PostprocessInput4MinMax : packoffset(c15.x);
	float4 PostprocessInput5MinMax : packoffset(c16.x);
	float4 PostprocessInput6MinMax : packoffset(c17.x);
	float4 PostprocessInput7MinMax : packoffset(c18.x);
	float4 PostprocessInput8MinMax : packoffset(c19.x);
	float4 PostprocessInput9MinMax : packoffset(c20.x);
	float4 PostprocessInput10MinMax : packoffset(c21.x);
	float4 ViewportSize : packoffset(c22.x);
	int4 ViewportRect : packoffset(c23.x);
	float4 ScreenPosToPixel : packoffset(c24.x);
	float4 SceneColorBufferUVViewport : packoffset(c25.x);
	float3 MappingPolynomial : packoffset(c26.x);
	float2 ViewportColor_Extent : packoffset(c27.x);
	float2 ViewportColor_ExtentInverse : packoffset(c27.z);
	float2 ViewportColor_ScreenPosToViewportScale : packoffset(c28.x);
	float2 ViewportColor_ScreenPosToViewportBias : packoffset(c28.z);
	int2 ViewportColor_ViewportMin : packoffset(c29.x);
	int2 ViewportColor_ViewportMax : packoffset(c29.z);
	float2 ViewportColor_ViewportSize : packoffset(c30.x);
	float2 ViewportColor_ViewportSizeInverse : packoffset(c30.z);
	float2 ViewportColor_UVViewportMin : packoffset(c31.x);
	float2 ViewportColor_UVViewportMax : packoffset(c31.z);
	float2 ViewportColor_UVViewportSize : packoffset(c32.x);
	float2 ViewportColor_UVViewportSizeInverse : packoffset(c32.z);
	float2 ViewportColor_UVViewportBilinearMin : packoffset(c33.x);
	float2 ViewportColor_UVViewportBilinearMax : packoffset(c33.z);
	float2 ViewportGlare_Extent : packoffset(c34.x);
	float2 ViewportGlare_ExtentInverse : packoffset(c34.z);
	float2 ViewportGlare_ScreenPosToViewportScale : packoffset(c35.x);
	float2 ViewportGlare_ScreenPosToViewportBias : packoffset(c35.z);
	int2 ViewportGlare_ViewportMin : packoffset(c36.x);
	int2 ViewportGlare_ViewportMax : packoffset(c36.z);
	float2 ViewportGlare_ViewportSize : packoffset(c37.x);
	float2 ViewportGlare_ViewportSizeInverse : packoffset(c37.z);
	float2 ViewportGlare_UVViewportMin : packoffset(c38.x);
	float2 ViewportGlare_UVViewportMax : packoffset(c38.z);
	float2 ViewportGlare_UVViewportSize : packoffset(c39.x);
	float2 ViewportGlare_UVViewportSizeInverse : packoffset(c39.z);
	float2 ViewportGlare_UVViewportBilinearMin : packoffset(c40.x);
	float2 ViewportGlare_UVViewportBilinearMax : packoffset(c40.z);
	float2 ViewportDestination_Extent : packoffset(c41.x);
	float2 ViewportDestination_ExtentInverse : packoffset(c41.z);
	float2 ViewportDestination_ScreenPosToViewportScale : packoffset(c42.x);
	float2 ViewportDestination_ScreenPosToViewportBias : packoffset(c42.z);
	int2 ViewportDestination_ViewportMin : packoffset(c43.x);
	int2 ViewportDestination_ViewportMax : packoffset(c43.z);
	float2 ViewportDestination_ViewportSize : packoffset(c44.x);
	float2 ViewportDestination_ViewportSizeInverse : packoffset(c44.z);
	float2 ViewportDestination_UVViewportMin : packoffset(c45.x);
	float2 ViewportDestination_UVViewportMax : packoffset(c45.z);
	float2 ViewportDestination_UVViewportSize : packoffset(c46.x);
	float2 ViewportDestination_UVViewportSizeInverse : packoffset(c46.z);
	float2 ViewportDestination_UVViewportBilinearMin : packoffset(c47.x);
	float2 ViewportDestination_UVViewportBilinearMax : packoffset(c47.z);
	float4 VignetteContext : packoffset(c48.x);
	float4 GlareContext : packoffset(c49.x);
	float4 NoiseContext : packoffset(c50.x);
	float4 HDRCompositionContext : packoffset(c51.x);
	float4 HDRCompositionContextColor : packoffset(c52.x);
	float4 CompositeSurfaceContext : packoffset(c53.x);
	float4 DeviceCorrectorContext : packoffset(c54.x);
	float4 DevelopmentContext : packoffset(c55.x);
};

cbuffer View : register(b1) 
{
	float4x4 View_TranslatedWorldToClip : packoffset(c0.x);
	float4x4 View_WorldToOrthographicClip : packoffset(c4.x);
	float4x4 View_TranslatedWorldToOrthographicClip : packoffset(c8.x);
	float4x4 View_WorldToClip : packoffset(c12.x);
	float4x4 View_ClipToWorld : packoffset(c16.x);
	float4x4 View_TranslatedWorldToView : packoffset(c20.x);
	float4x4 View_ViewToTranslatedWorld : packoffset(c24.x);
	float4x4 View_TranslatedWorldToCameraView : packoffset(c28.x);
	float4x4 View_CameraViewToTranslatedWorld : packoffset(c32.x);
	float4x4 View_ViewToClip : packoffset(c36.x);
	float4x4 View_ViewToClipNoAA : packoffset(c40.x);
	float4x4 View_ClipToView : packoffset(c44.x);
	float4x4 View_ClipToTranslatedWorld : packoffset(c48.x);
	float4x4 View_SVPositionToTranslatedWorld : packoffset(c52.x);
	float4x4 View_ScreenToWorld : packoffset(c56.x);
	float4x4 View_ScreenToTranslatedWorld : packoffset(c60.x);
	float4x4 View_MobileMultiviewShadowTransform : packoffset(c64.x);
	float3 View_ViewForward : packoffset(c68.x);
	float PrePadding_View_1100 : packoffset(c68.w);
	float3 View_ViewUp : packoffset(c69.x);
	float PrePadding_View_1116 : packoffset(c69.w);
	float3 View_ViewRight : packoffset(c70.x);
	float PrePadding_View_1132 : packoffset(c70.w);
	float3 View_HMDViewNoRollUp : packoffset(c71.x);
	float PrePadding_View_1148 : packoffset(c71.w);
	float3 View_HMDViewNoRollRight : packoffset(c72.x);
	float PrePadding_View_1164 : packoffset(c72.w);
	float4 View_InvDeviceZToWorldZTransform : packoffset(c73.x);
	float4 View_ScreenPositionScaleBias : packoffset(c74.x);
	float3 View_WorldCameraOrigin : packoffset(c75.x);
	float PrePadding_View_1212 : packoffset(c75.w);
	float3 View_TranslatedWorldCameraOrigin : packoffset(c76.x);
	float PrePadding_View_1228 : packoffset(c76.w);
	float3 View_WorldViewOrigin : packoffset(c77.x);
	float PrePadding_View_1244 : packoffset(c77.w);
	float3 View_PreViewTranslation : packoffset(c78.x);
	float PrePadding_View_1260 : packoffset(c78.w);
	float4x4 View_PrevProjection : packoffset(c79.x);
	float4x4 View_PrevViewProj : packoffset(c83.x);
	float4x4 View_PrevViewRotationProj : packoffset(c87.x);
	float4x4 View_PrevViewToClip : packoffset(c91.x);
	float4x4 View_PrevClipToView : packoffset(c95.x);
	float4x4 View_PrevTranslatedWorldToClip : packoffset(c99.x);
	float4x4 View_PrevWorldToOrthographicClip : packoffset(c103.x);
	float4x4 View_PrevTranslatedWorldToOrthographicClip : packoffset(c107.x);
	float4x4 View_PrevTranslatedWorldToView : packoffset(c111.x);
	float4x4 View_PrevViewToTranslatedWorld : packoffset(c115.x);
	float4x4 View_PrevTranslatedWorldToCameraView : packoffset(c119.x);
	float4x4 View_PrevCameraViewToTranslatedWorld : packoffset(c123.x);
	float3 View_PrevWorldCameraOrigin : packoffset(c127.x);
	float PrePadding_View_2044 : packoffset(c127.w);
	float3 View_PrevWorldViewOrigin : packoffset(c128.x);
	float PrePadding_View_2060 : packoffset(c128.w);
	float3 View_PrevPreViewTranslation : packoffset(c129.x);
	float PrePadding_View_2076 : packoffset(c129.w);
	float4x4 View_PrevInvViewProj : packoffset(c130.x);
	float4x4 View_PrevScreenToTranslatedWorld : packoffset(c134.x);
	float4x4 View_ClipToPrevClip : packoffset(c138.x);
	float4x4 View_ClipToPrevClipWithoutTranslation : packoffset(c142.x);
	float4x4 View_ProjectionToWorld : packoffset(c146.x);
	float4x4 View_WorldToProjection : packoffset(c150.x);
	float4 View_TemporalAAJitter : packoffset(c154.x);
	float4 View_TemporalSamplerBias : packoffset(c155.x);
	float4 View_GlobalClippingPlane : packoffset(c156.x);
	float2 View_FieldOfViewWideAngles : packoffset(c157.x);
	float2 View_PrevFieldOfViewWideAngles : packoffset(c157.z);
	float4 View_ViewRectMin : packoffset(c158.x);
	float4 View_ViewSizeAndInvSize : packoffset(c159.x);
	float4 View_LightProbeSizeRatioAndInvSizeRatio : packoffset(c160.x);
	float4 View_BufferSizeAndInvSize : packoffset(c161.x);
	float4 View_BufferBilinearUVMinMax : packoffset(c162.x);
	float4 View_ScreenToViewSpace : packoffset(c163.x);
	int View_NumSceneColorMSAASamples : packoffset(c164.x);
	float View_PreExposure : packoffset(c164.y);
	float View_OneOverPreExposure : packoffset(c164.z);
	float View_PreviousPreExposure : packoffset(c164.w);
	float View_PreviousOneOverPreExposure : packoffset(c165.x);
	float PrePadding_View_2644 : packoffset(c165.y);
	float PrePadding_View_2648 : packoffset(c165.z);
	float PrePadding_View_2652 : packoffset(c165.w);
	float4 View_DiffuseOverrideParameter : packoffset(c166.x);
	float4 View_SpecularOverrideParameter : packoffset(c167.x);
	float4 View_NormalOverrideParameter : packoffset(c168.x);
	float4 View_RoughnessOverrideParameter : packoffset(c169.x);
	float View_PrevFrameGameTime : packoffset(c170.x);
	float View_PrevFrameRealTime : packoffset(c170.y);
	float View_OutOfBoundsMask : packoffset(c170.z);
	float PrePadding_View_2732 : packoffset(c170.w);
	float3 View_WorldCameraMovementSinceLastFrame : packoffset(c171.x);
	float View_CullingSign : packoffset(c171.w);
	float View_NearPlane : packoffset(c172.x);
	float View_AdaptiveTessellationFactor : packoffset(c172.y);
	float View_GameTime : packoffset(c172.z);
	float View_RealTime : packoffset(c172.w);
	float View_DeltaTime : packoffset(c173.x);
	float View_EnvironmentTime : packoffset(c173.y);
	float View_PreviousEnvironmentTime : packoffset(c173.z);
	float View_MaterialTextureMipBias : packoffset(c173.w);
	float View_MaterialTextureDerivativeMultiply : packoffset(c174.x);
	int View_Random : packoffset(c174.y);
	int View_FrameNumber : packoffset(c174.z);
	int View_StateFrameIndexMod8 : packoffset(c174.w);
	int View_StateFrameIndex : packoffset(c175.x);
	int View_StateFrameDelayIndex : packoffset(c175.y);
	int View_DebugViewModeMask : packoffset(c175.z);
	float View_CameraCut : packoffset(c175.w);
	float View_UnlitViewmodeMask : packoffset(c176.x);
	float PrePadding_View_2820 : packoffset(c176.y);
	float PrePadding_View_2824 : packoffset(c176.z);
	float PrePadding_View_2828 : packoffset(c176.w);
	float4 View_DirectionalLightColor : packoffset(c177.x);
	float3 View_DirectionalLightDirection : packoffset(c178.x);
	float PrePadding_View_2860 : packoffset(c178.w);
	float4 View_TranslucencyLightingVolumeMin[2] : packoffset(c179.x);
	float4 View_TranslucencyLightingVolumeInvSize[2] : packoffset(c181.x);
	float4 View_TranslucencyLightingVolumeDistance[2] : packoffset(c183.x);
	float4 View_TemporalAAParams : packoffset(c185.x);
	float4 View_CircleDOFParams : packoffset(c186.x);
	int View_ForceDrawAllVelocities : packoffset(c187.x);
	float View_DepthOfFieldIntensity : packoffset(c187.y);
	float View_DepthOfFieldFocalDistance : packoffset(c187.z);
	float View_DepthOfFieldFstop : packoffset(c187.w);
	float View_LightModifierEnvironmentLight : packoffset(c188.x);
	float View_LightModifierDirectionalLight : packoffset(c188.y);
	float View_MotionBlurNormalizedToPixel : packoffset(c188.z);
	float View_bSubsurfacePostprocessEnabled : packoffset(c188.w);
	float4 View_GeneralPurposeTweak : packoffset(c189.x);
	float View_DemosaicVposOffset : packoffset(c190.x);
	float PrePadding_View_3044 : packoffset(c190.y);
	float PrePadding_View_3048 : packoffset(c190.z);
	float PrePadding_View_3052 : packoffset(c190.w);
	float3 View_IndirectLightingColorScale : packoffset(c191.x);
	float View_AtmosphericFogSunPower : packoffset(c191.w);
	float View_AtmosphericFogPower : packoffset(c192.x);
	float View_AtmosphericFogDensityScale : packoffset(c192.y);
	float View_AtmosphericFogDensityOffset : packoffset(c192.z);
	float View_AtmosphericFogGroundOffset : packoffset(c192.w);
	float View_AtmosphericFogDistanceScale : packoffset(c193.x);
	float View_AtmosphericFogAltitudeScale : packoffset(c193.y);
	float View_AtmosphericFogHeightScaleRayleigh : packoffset(c193.z);
	float View_AtmosphericFogStartDistance : packoffset(c193.w);
	float View_AtmosphericFogDistanceOffset : packoffset(c194.x);
	float View_AtmosphericFogSunDiscScale : packoffset(c194.y);
	float PrePadding_View_3112 : packoffset(c194.z);
	float PrePadding_View_3116 : packoffset(c194.w);
	float4 View_AtmosphereLightDirection[2] : packoffset(c195.x);
	float4 View_AtmosphereLightColor[2] : packoffset(c197.x);
	float4 View_AtmosphereLightColorGlobalPostTransmittance[2] : packoffset(c199.x);
	float4 View_AtmosphereLightDiscLuminance[2] : packoffset(c201.x);
	float4 View_AtmosphereLightDiscCosHalfApexAngle[2] : packoffset(c203.x);
	float4 View_SkyViewLutSizeAndInvSize : packoffset(c205.x);
	float3 View_SkyWorldCameraOrigin : packoffset(c206.x);
	float PrePadding_View_3308 : packoffset(c206.w);
	float4 View_SkyPlanetCenterAndViewHeight : packoffset(c207.x);
	float4x4 View_SkyViewLutReferential : packoffset(c208.x);
	float4 View_SkyAtmosphereSkyLuminanceFactor : packoffset(c212.x);
	float View_SkyAtmospherePresentInScene : packoffset(c213.x);
	float View_SkyAtmosphereHeightFogContribution : packoffset(c213.y);
	float View_SkyAtmosphereBottomRadiusKm : packoffset(c213.z);
	float View_SkyAtmosphereTopRadiusKm : packoffset(c213.w);
	float4 View_SkyAtmosphereCameraAerialPerspectiveVolumeSizeAndInvSize : packoffset(c214.x);
	float View_SkyAtmosphereAerialPerspectiveStartDepthKm : packoffset(c215.x);
	float View_SkyAtmosphereCameraAerialPerspectiveVolumeDepthResolution : packoffset(c215.y);
	float View_SkyAtmosphereCameraAerialPerspectiveVolumeDepthResolutionInv : packoffset(c215.z);
	float View_SkyAtmosphereCameraAerialPerspectiveVolumeDepthSliceLengthKm : packoffset(c215.w);
	float View_SkyAtmosphereCameraAerialPerspectiveVolumeDepthSliceLengthKmInv : packoffset(c216.x);
	float View_SkyAtmosphereApplyCameraAerialPerspectiveVolume : packoffset(c216.y);
	int View_AtmosphericFogRenderMask : packoffset(c216.z);
	int View_AtmosphericFogInscatterAltitudeSampleNum : packoffset(c216.w);
	float4 View_EnvironmentLightFallbackContext : packoffset(c217.x);
	float4 View_FogContextDirectionalLightDirection : packoffset(c218.x);
	float4 View_FogContextDirectionalLightColor : packoffset(c219.x);
	float View_FogContextMediumIOR : packoffset(c220.x);
	int View_NumberOfFogContext : packoffset(c220.y);
	int PrePadding_View_3528 : packoffset(c220.z);
	int PrePadding_View_3532 : packoffset(c220.w);
	float4 View_FogContextHeightBasedContextAlbedo[4] : packoffset(c221.x);
	float4 View_FogContextHeightBasedContextDensity[4] : packoffset(c225.x);
	float4 View_FogContextTransmitContextAlbedo[4] : packoffset(c229.x);
	float4 View_FogContextTransmitContextDensity[4] : packoffset(c233.x);
	float4 View_FogContextRegionContext[4] : packoffset(c237.x);
	float4 View_FogContextLightContext[4] : packoffset(c241.x);
	float3 View_NormalCurvatureToRoughnessScaleBias : packoffset(c245.x);
	float View_RenderingReflectionCaptureMask : packoffset(c245.w);
	float View_RealTimeReflectionCapture : packoffset(c246.x);
	float View_RealTimeReflectionCapturePreExposure : packoffset(c246.y);
	float PrePadding_View_3944 : packoffset(c246.z);
	float PrePadding_View_3948 : packoffset(c246.w);
	float4 View_AmbientCubemapTint : packoffset(c247.x);
	float View_AmbientCubemapIntensity : packoffset(c248.x);
	float View_WetnessIntensity : packoffset(c248.y);
	float View_SkyLightApplyPrecomputedBentNormalShadowingFlag : packoffset(c248.z);
	float View_SkyLightAffectReflectionFlag : packoffset(c248.w);
	float View_SkyLightAffectGlobalIlluminationFlag : packoffset(c249.x);
	float PrePadding_View_3988 : packoffset(c249.y);
	float PrePadding_View_3992 : packoffset(c249.z);
	float PrePadding_View_3996 : packoffset(c249.w);
	float4 View_SkyLightColor : packoffset(c250.x);
	float4 View_MobileSkyIrradianceEnvironmentMap[7] : packoffset(c251.x);
	float View_MobilePreviewMode : packoffset(c258.x);
	float View_HMDEyePaddingOffset : packoffset(c258.y);
	float View_ReflectionCubemapMaxMip : packoffset(c258.z);
	float View_ShowDecalsMask : packoffset(c258.w);
	int View_DistanceFieldAOSpecularOcclusionMode : packoffset(c259.x);
	float View_IndirectCapsuleSelfShadowingIntensity : packoffset(c259.y);
	float PrePadding_View_4152 : packoffset(c259.z);
	float PrePadding_View_4156 : packoffset(c259.w);
	float3 View_ReflectionEnvironmentRoughnessMixingScaleBiasAndLargestWeight : packoffset(c260.x);
	int View_StereoPassIndex : packoffset(c260.w);
	float4 View_GlobalVolumeCenterAndExtent[4] : packoffset(c261.x);
	float4 View_GlobalVolumeWorldToUVAddAndMul[4] : packoffset(c265.x);
	float View_GlobalVolumeDimension : packoffset(c269.x);
	float View_GlobalVolumeTexelSize : packoffset(c269.y);
	float View_MaxGlobalDistance : packoffset(c269.z);
	float PrePadding_View_4316 : packoffset(c269.w);
	int2 View_CursorPosition : packoffset(c270.x);
	float View_bCheckerboardSubsurfaceProfileRendering : packoffset(c270.z);
	float PrePadding_View_4332 : packoffset(c270.w);
	float3 View_PrecomputedLightVolumeGridZParams : packoffset(c271.x);
	float PrePadding_View_4348 : packoffset(c271.w);
	float3 View_PrecomputedLightVolumeGridSize : packoffset(c272.x);
	float PrePadding_View_4364 : packoffset(c272.w);
	int3 View_PrecomputedLightVolumeGridSizeInt : packoffset(c273.x);
	int PrePadding_View_4380 : packoffset(c273.w);
	float3 View_VolumetricFogGridSize : packoffset(c274.x);
	float PrePadding_View_4396 : packoffset(c274.w);
	float3 View_VolumetricFogGridSizeReciprocal : packoffset(c275.x);
	float PrePadding_View_4412 : packoffset(c275.w);
	float3 View_VolumetricFogGridZParameter : packoffset(c276.x);
	float PrePadding_View_4428 : packoffset(c276.w);
	float3 View_VolumetricFogGridZParameterReciprocal : packoffset(c277.x);
	float PrePadding_View_4444 : packoffset(c277.w);
	float3 View_VolumetricFogGridCoordinateSolver : packoffset(c278.x);
	float PrePadding_View_4460 : packoffset(c278.w);
	float3 View_VolumetricFogGridCoordinateMinimum : packoffset(c279.x);
	float PrePadding_View_4476 : packoffset(c279.w);
	float3 View_VolumetricFogGridCoordinateMaximum : packoffset(c280.x);
	float View_VolumetricFogMaxDistance : packoffset(c280.w);
	float3 View_VolumetricLightmapWorldToUVScale : packoffset(c281.x);
	float PrePadding_View_4508 : packoffset(c281.w);
	float3 View_VolumetricLightmapWorldToUVAdd : packoffset(c282.x);
	float PrePadding_View_4524 : packoffset(c282.w);
	float3 View_VolumetricLightmapIndirectionTextureSize : packoffset(c283.x);
	float View_VolumetricLightmapBrickSize : packoffset(c283.w);
	float3 View_VolumetricLightmapBrickTexelSize : packoffset(c284.x);
	float View_StereoIPD : packoffset(c284.w);
	float View_IndirectLightingCacheShowFlag : packoffset(c285.x);
	float View_EyeToPixelSpreadAngle : packoffset(c285.y);
	float PrePadding_View_4568 : packoffset(c285.z);
	float PrePadding_View_4572 : packoffset(c285.w);
	float4x4 View_WorldToVirtualTexture : packoffset(c286.x);
	float4 View_XRPassthroughCameraUVs[2] : packoffset(c290.x);
	int View_VirtualTextureFeedbackStride : packoffset(c292.x);
	int PrePadding_View_4676 : packoffset(c292.y);
	int PrePadding_View_4680 : packoffset(c292.z);
	int PrePadding_View_4684 : packoffset(c292.w);
	float4 View_RuntimeVirtualTextureMipLevel : packoffset(c293.x);
	float2 View_RuntimeVirtualTexturePackHeight : packoffset(c294.x);
	float PrePadding_View_4712 : packoffset(c294.z);
	float PrePadding_View_4716 : packoffset(c294.w);
	float4 View_RuntimeVirtualTextureDebugParams : packoffset(c295.x);
	int View_FarShadowStaticMeshLODBias : packoffset(c296.x);
	float View_MinRoughness : packoffset(c296.y);
	float PrePadding_View_4744 : packoffset(c296.z);
	float PrePadding_View_4748 : packoffset(c296.w);
	float4 View_HairRenderInfo : packoffset(c297.x);
	int View_EnableSkyLight : packoffset(c298.x);
	int View_HairRenderInfoBits : packoffset(c298.y);
	int View_HairComponents : packoffset(c298.z);
	int View_DebugContext : packoffset(c298.w);
	int View_DebugTweak : packoffset(c299.x);
};

struct InputStruct 
{
	float2 param0 : TEXCOORD0;
	float4 param1 : TEXCOORD1;
	float4 Position : SV_Position;
};

//|||||||||||||||||||||||||||||||||| COLOR TRANSFER FUNCTIONS ||||||||||||||||||||||||||||||||||
//|||||||||||||||||||||||||||||||||| COLOR TRANSFER FUNCTIONS ||||||||||||||||||||||||||||||||||
//|||||||||||||||||||||||||||||||||| COLOR TRANSFER FUNCTIONS ||||||||||||||||||||||||||||||||||

float3 EncodePQ(float3 linearColor)
{
    //the game converts its linear color to a PQ LUT coordinate with the ST 2084 constants
    float3 normalizedColor = saturate(linearColor * 0.01f);

    float3 luminancePower = pow(normalizedColor, 0.1593017578125f);

    float3 colorNumerator = 0.8359375f + 18.8515625f * luminancePower;

    float3 colorDenominator = 1.0f + 18.6875f * luminancePower;

    return saturate(pow(max(colorNumerator / colorDenominator, 0.0f), 78.84375f));
}

float DecodeSRGBChannel(float encodedChannel)
{
    //the second LUT returns sRGB, but the composite layers are combined in linear light
    if(encodedChannel < 0.04045f)
        return encodedChannel * 0.0773993805f;

    return pow((encodedChannel + 0.055f) * 0.9478673339f, 2.4f);
}

float EncodeSRGBChannel(float linearChannel)
{
    //the original shader saturates the linear composite before this piecewise transfer
    if(linearChannel < 0.0031308f)
        return linearChannel * 12.92f;

    return 1.055f * pow(linearChannel, 0.4166666667f) - 0.055f;
}

//|||||||||||||||||||||||||||||||||| FINAL POST PROCESS ||||||||||||||||||||||||||||||||||
//|||||||||||||||||||||||||||||||||| FINAL POST PROCESS ||||||||||||||||||||||||||||||||||
//|||||||||||||||||||||||||||||||||| FINAL POST PROCESS ||||||||||||||||||||||||||||||||||

float4 EditedShaderPS(noperspective in InputStruct IN) : SV_Target0
{
    //the noise volume uses a 128 by 128 pixel tile and a 64 frame cycle
    int2 noisePixel = int2(IN.Position.xy) & 127;

    int noiseFrame = View_StateFrameIndex & 63;

    float blueNoise = View_SpatiotemporalBlueNoiseVolumeTexture.Load(int4(noisePixel, noiseFrame, 0)).r;

    //convert the destination pixel to a local UV; a device setting optionally snaps to 2 by 2 pixel centers
    float2 destinationPixel = IN.Position.xy - float2(ViewportDestination_ViewportMin);

    float2 destinationUV = saturate(destinationPixel * ViewportDestination_ViewportSizeInverse);

    float2 sampleDestinationUV = destinationUV;

    if(DeviceCorrectorContext.z != 0.0f)
    {
        float2 snappedDestinationPixel = floor(destinationPixel * 0.5f) * 2.0f + 1.0f;

        sampleDestinationUV = saturate(snappedDestinationPixel * ViewportDestination_ViewportSizeInverse);
    }

    //the scene color and glare each have their own atlas viewport and bilinear-safe UV limits
    float2 colorPixel = float2(ViewportColor_ViewportMin) + ViewportColor_ViewportSize * sampleDestinationUV;

    float2 colorUV = clamp(colorPixel * ViewportColor_ExtentInverse, ViewportColor_UVViewportBilinearMin, ViewportColor_UVViewportBilinearMax);

    float2 glarePixel = float2(ViewportGlare_ViewportMin) + ViewportGlare_ViewportSize * sampleDestinationUV;

    float2 glareUV = clamp(glarePixel * ViewportGlare_ExtentInverse, ViewportGlare_UVViewportBilinearMin, ViewportGlare_UVViewportBilinearMax);

    //the SDR layers use the unsnapped UV and fit a 16:9 composition into the destination viewport
    float2 compositeAspectScale = float2(min(ViewportDestination_ViewportSize.x * 0.5625f * ViewportDestination_ViewportSizeInverse.y, 1.0f), min(ViewportDestination_ViewportSize.y * 1.777777791f * ViewportDestination_ViewportSizeInverse.x, 1.0f));

    float2 compositeUV = 0.5f + (destinationUV - 0.5f) * compositeAspectScale;

    //the vignette uses the incoming view ray; the color input is capped before this multiplier
    float vignetteDepth = IN.param1.z / max(VignetteContext.y, 0.001f);

    float vignetteDepthSquared = vignetteDepth * vignetteDepth;

    float vignetteRadiusSquared = max(dot(float3(IN.param1.xy, vignetteDepth), float3(IN.param1.xy, vignetteDepth)), 0.00001f);

    float vignetteRatio = vignetteDepthSquared / vignetteRadiusSquared;

    float vignetteMultiplier = 1.0f + VignetteContext.x * (vignetteRatio * vignetteRatio - 1.0f);

    float3 sceneColor = min(ColorTexture.SampleLevel(View_SharedBilinearClampedSampler, colorUV, 0.0f).rgb, 64512.0f) * vignetteMultiplier;

    float3 glareColor = min(GlareTexture.SampleLevel(View_SharedBilinearClampedSampler, glareUV, 0.0f).rgb, 64512.0f);

    float3 combinedColor = sceneColor + GlareContext.x * (glareColor - sceneColor + GlareContext.y * sceneColor);

    //each 32-cube LUT is addressed at texel centers, so its first and last cells stay reachable
    float3 colorInBT709PQ = EncodePQ(combinedColor);

    float3 coordinatesForBT2020LUT = colorInBT709PQ * 0.96875f + 0.015625f;

    float3 colorInBT2020PQ = BT709PQToBT2020PQLUT.SampleLevel(View_SharedBilinearClampedSampler, coordinatesForBT2020LUT, 0.0f).rgb;

    float3 coordinatesForSRGBLUT = colorInBT2020PQ * 0.96875f + 0.015625f;

    float3 colorFromSRGBLUT = BT2020PQTosRGBLUT.SampleLevel(View_SharedBilinearClampedSampler, coordinatesForSRGBLUT, 0.0f).rgb;

    float3 linearColorFromLUT = float3(DecodeSRGBChannel(colorFromSRGBLUT.r), DecodeSRGBChannel(colorFromSRGBLUT.g), DecodeSRGBChannel(colorFromSRGBLUT.b));

    //the three SDR layers accumulate RGB directly and multiply their alpha values for the LUT contribution
    float4 backgroundColor = CompositeSDRBackgroundTexture.SampleLevel(View_SharedBilinearClampedSampler, compositeUV, 0.0f);

    float4 compositeColor = CompositeSDRTexture.SampleLevel(View_SharedBilinearClampedSampler, compositeUV, 0.0f);

    float4 foregroundColor = CompositeSDRForegroundTexture.SampleLevel(View_SharedBilinearClampedSampler, compositeUV, 0.0f);

    float3 compositedLayerColor = foregroundColor.rgb + foregroundColor.a * (compositeColor.rgb + compositeColor.a * backgroundColor.rgb);

    float compositeOpacity = foregroundColor.a * compositeColor.a * backgroundColor.a;

    float3 linearOutputColor = saturate(compositedLayerColor + compositeOpacity * linearColorFromLUT);

    float3 encodedOutputColor = float3(EncodeSRGBChannel(linearOutputColor.r), EncodeSRGBChannel(linearOutputColor.g), EncodeSRGBChannel(linearOutputColor.b));

    //shape the blue noise and avoid dithering values very close to either output endpoint
    float centeredNoise = blueNoise * 2.0f - 1.0f;

    float noiseOffset = sign(centeredNoise) * (1.0f - sqrt(1.0f - abs(centeredNoise))) * (1.0f / 255.0f);

    if(abs(encodedOutputColor.r * 2.0f - 1.0f) < 253.0f / 255.0f)
        encodedOutputColor.r += noiseOffset;

    if(abs(encodedOutputColor.g * 2.0f - 1.0f) < 253.0f / 255.0f)
        encodedOutputColor.g += noiseOffset;

    if(abs(encodedOutputColor.b * 2.0f - 1.0f) < 253.0f / 255.0f)
        encodedOutputColor.b += noiseOffset;

    return float4(saturate(encodedOutputColor), 0.0f);
}
