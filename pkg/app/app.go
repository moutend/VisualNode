package app

import (
	"context"
	"fmt"
	"net/http"
	"sync"

	"github.com/moutend/VisualNode/pkg/api"
	"github.com/moutend/VisualNode/pkg/mux"
)

type app struct {
	m         *sync.Mutex
	wg        *sync.WaitGroup
	server    *http.Server
	isRunning bool
}

func (a *app) setup() error {
	mux := mux.New()

	mux.Get("/v1/visual/enable", api.GetVisualEnable)
	mux.Get("/v1/visual/disable", api.GetVisualDisable)
	mux.Post("/v1/visual/highlight", api.PostVisualHighlight)
	mux.Post("/v1/visual/text", api.PostVisualText)

	a.server = &http.Server{
		Addr:    ":7904",
		Handler: mux,
	}

	a.wg.Add(1)

	go func() {
		if err := a.server.ListenAndServe(); err != http.ErrServerClosed {
			panic(err)
		}
		a.wg.Done()
	}()

	return nil
}

func (a *app) Setup() error {
	a.m.Lock()
	defer a.m.Unlock()

	if a.isRunning {
		fmt.Errorf("Setup is already done")
	}
	if err := a.setup(); err != nil {
		return err
	}

	a.isRunning = true

	return nil
}

func (a *app) teardown() error {
	if err := a.server.Shutdown(context.TODO()); err != nil {
		return err
	}

	a.wg.Wait()

	return nil
}

func (a *app) Teardown() error {
	a.m.Lock()
	defer a.m.Unlock()

	if !a.isRunning {
		return fmt.Errorf("Teardown is already done")
	}
	if err := a.teardown(); err != nil {
		return err
	}

	a.isRunning = false

	return nil
}

func New() *app {
	return &app{
		m:  &sync.Mutex{},
		wg: &sync.WaitGroup{},
	}
}
