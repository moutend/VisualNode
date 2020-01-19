#pragma once

typedef struct {
  float Red = 0;
  float Green = 0;
  float Blue = 0;
  float Alpha = 0;
} RGBAColor;

typedef struct {
  float Left = 0;
  float Top = 0;
  float Width = 0;
  float Height = 0;
  float Radius;
  float BorderThickness;
  RGBAColor *BorderColor;
} HighlightRectangle;
