package api

import (
	"bytes"
	"encoding/json"
	"fmt"
	"io"
	"log"
	"math"
	"net/http"
	"unsafe"

	"github.com/moutend/VisualNode/pkg/dll"
	"github.com/moutend/VisualNode/pkg/types"
)

func GetVisualEnable(w http.ResponseWriter, r *http.Request) error {
	var code int32

	dll.ProcSetup.Call(uintptr(unsafe.Pointer(&code)), uintptr(0))

	if code != 0 {
		log.Printf("Failed to call Setup()")
		return fmt.Errorf("Internal error")
	}
	if _, err := io.WriteString(w, "{}"); err != nil {
		log.Println(err)
		return fmt.Errorf("Internal error")
	}
	return nil
}

func GetVisualDisable(w http.ResponseWriter, r *http.Request) error {
	var code int32

	dll.ProcTeardown.Call(uintptr(unsafe.Pointer(&code)))

	if code != 0 {
		log.Println("Failed to call Teardown (code=%v)", code)
		return fmt.Errorf("Internal error")
	}
	if _, err := io.WriteString(w, "{}"); err != nil {
		log.Println(err)
		return fmt.Errorf("Internal error")
	}

	return nil
}

type postVisualHighlightRequest struct {
	Left            float32 `json:"left"`
	Top             float32 `json:"top"`
	Width           float32 `json:"width"`
	Height          float32 `json:"height"`
	Radius          float32 `json:"radius"`
	BorderThickness float32 `json:"borderThickness"`
	BorderColor     struct {
		Red   float32 `json:"red"`
		Green float32 `json:"green"`
		Blue  float32 `json:"blue"`
		Alpha float32 `json:"alpha"`
	} `json:"borderColor"`
}

func PostVisualHighlight(w http.ResponseWriter, r *http.Request) error {
	log.Println("Called PostVisualHighlight")

	var code int32
	var req postVisualHighlightRequest

	buffer := &bytes.Buffer{}

	if _, err := io.Copy(buffer, r.Body); err != nil {
		log.Println(err)
		return err
	}
	if err := json.Unmarshal(buffer.Bytes(), &req); err != nil {
		log.Println(err)
		return err
	}

	log.Printf("req=%+v\n", req)

	borderColor := types.RGBAColor{}

	borderColor.Red = math.Float32bits(req.BorderColor.Red)
	borderColor.Green = math.Float32bits(req.BorderColor.Green)
	borderColor.Blue = math.Float32bits(req.BorderColor.Blue)
	borderColor.Alpha = math.Float32bits(req.BorderColor.Alpha)

	rect := types.HighlightRectangle{}

	rect.Left = math.Float32bits(req.Left)
	rect.Top = math.Float32bits(req.Top)
	rect.Width = math.Float32bits(req.Width)
	rect.Height = math.Float32bits(req.Height)
	rect.Radius = math.Float32bits(req.Radius)
	rect.BorderThickness = math.Float32bits(req.BorderThickness)
	rect.BorderColor = uintptr(unsafe.Pointer(&borderColor))

	dll.ProcSetHighlightRectangle.Call(uintptr(unsafe.Pointer(&code)), uintptr(unsafe.Pointer(&rect)))

	if code != 0 {
		log.Println("Failed to call SetHighlightRectangle (code=%v)", code)
		return fmt.Errorf("Internal error")
	}
	if _, err := io.WriteString(w, "{}"); err != nil {
		log.Println(err)
		return fmt.Errorf("Internal error")
	}

	return nil
}
