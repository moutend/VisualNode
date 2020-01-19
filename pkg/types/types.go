package types

type HighlightRectangle struct {
	Left            uint32
	Top             uint32
	Width           uint32
	Height          uint32
	Radius          uint32
	BorderThickness uint32
	BorderColor     uintptr
}

type RGBAColor struct {
	Red   uint32
	Green uint32
	Blue  uint32
	Alpha uint32
}
