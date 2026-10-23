// HardwareTestMenu: the Settings > Hardware Test list (built by createMenu). Include after TextMenu.h.
#pragma once
class HardwareTestMenu : public TextMenu
{
public:
    HardwareTestMenu();
    ~HardwareTestMenu() override;
    int onSelect() override;
    void drawList(SDL_Surface *) override;
    void freeListItem() override;
    void initializeTests();
    int stressSeconds;
};
