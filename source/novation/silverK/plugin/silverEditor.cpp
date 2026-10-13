#include "silverEditor.h"
#include "silverBrowser.h"
#include "BinaryData.h"
#include <cmath>

namespace silverK
{
namespace
{
using Spec = ksPlugin::Control;
const juce::Colour silver(0xffced1d5), ink(0xff171b20), blue(0xff087fe5), dark(0xff24262a), light(0xfff1f4f7);
juce::Rectangle<int> rect(const juce::var& v)
{
    auto* a=v.getArray();
    return a && a->size()==4 ? juce::Rectangle<int>(int((*a)[0]),int((*a)[1]),int((*a)[2]),int((*a)[3])) : juce::Rectangle<int>{};
}
juce::var skinData(Processor& p)
{
    int size=0;const auto* bytes=BinaryData::getNamedResource("silverk_json",size);
    auto skin=juce::JSON::parse(juce::String::fromUTF8(bytes,size));
    if(p.skinChoice=="Custom" && juce::File(p.customSkin).existsAsFile()){
      auto custom=juce::JSON::parse(juce::File(p.customSkin));
      if(custom.isObject() && custom["controls"].isArray() && custom["effects"].isArray() && int(custom["width"])==1280 && int(custom["height"])==752)skin=custom;
    }
    return skin;
}
class SilverLook final : public juce::LookAndFeel_V4
{
    juce::Image knob,grip;
public:
    SilverLook()
    {
        int size=0; const auto* data=BinaryData::getNamedResource("silverkknobbody_png",size);
        knob=juce::ImageFileFormat::loadFrom(data,size);
        data=BinaryData::getNamedResource("silverkknobgrip_png",size);
        grip=juce::ImageFileFormat::loadFrom(data,size);
        setColour(juce::PopupMenu::backgroundColourId,juce::Colour(0xff202a34));
        setColour(juce::PopupMenu::textColourId,light);
        setColour(juce::PopupMenu::highlightedBackgroundColourId,blue);
        setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);
        setColour(juce::Slider::textBoxBackgroundColourId,juce::Colours::transparentBlack);
        setColour(juce::ComboBox::textColourId,light);
    }
    juce::Label* createSliderTextBox(juce::Slider& slider) override
    {
        auto* field=LookAndFeel_V4::createSliderTextBox(slider);
        field->setColour(juce::Label::outlineColourId,juce::Colours::transparentBlack);
        field->setColour(juce::Label::outlineWhenEditingColourId,juce::Colours::transparentBlack);
        field->setColour(juce::TextEditor::outlineColourId,juce::Colours::transparentBlack);
        field->setColour(juce::TextEditor::focusedOutlineColourId,juce::Colours::transparentBlack);
        field->setColour(juce::TextEditor::backgroundColourId,juce::Colours::transparentBlack);
        return field;
    }
    void drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,float pos,float start,float end,juce::Slider& slider) override
    {
        const float available=float(juce::jmin(w,h))-4;
        const float requested=slider.getProperties().contains("rotaryDiameter")?float(slider.getProperties()["rotaryDiameter"]):available;
        const float d=juce::jmin(available,requested),cx=x+w*.5f,cy=y+h*.5f;
        const float position=juce::jlimit(0.f,1.f,pos);
        const float angle=start+position*(end-start);
        if(slider.getProperties()["rotaryStyle"].toString()=="classic")
        {
            // The same cap size as the main knobs, with a smooth bevel instead
            // of the rubber grip and LED surround. Its lighting remains fixed.
            g.setGradientFill(juce::ColourGradient(juce::Colour(0xff737a82),cx-d*.3f,cy-d*.4f,
                juce::Colour(0xff10141a),cx+d*.3f,cy+d*.4f,false));
            g.fillEllipse(cx-d*.5f,cy-d*.5f,d,d);
            const float faceDiameter=d*.91f,cap=faceDiameter/.674f;
            if(knob.isValid())
            {
                juce::Graphics::ScopedSaveState saved(g);
                juce::Path face;face.addEllipse(cx-faceDiameter*.5f,cy-faceDiameter*.5f,faceDiameter,faceDiameter);g.reduceClipRegion(face);
                g.drawImageTransformed(knob,juce::AffineTransform::scale(cap/knob.getWidth(),cap/knob.getHeight())
                    .followedBy(juce::AffineTransform::translation(cx-cap*.5f,cy-cap*.5f)));
            }
            const float markerWidth=juce::jmax(1.5f,d*.025f);
            juce::Path marker;marker.addRoundedRectangle(-markerWidth*.5f,-d*.39f,markerWidth,d*.17f,markerWidth*.5f);
            g.setColour(light);g.fillPath(marker,juce::AffineTransform::rotation(angle).translated(cx,cy));
            return;
        }
        g.setColour(juce::Colour(0xff10151c));g.fillEllipse(cx-d*.5f,cy-d*.5f,d,d);
        for(int i=0;i<21;++i)
        {
            const float a=start+(end-start)*i/20.f,r=d*.45f;
            // Do not illuminate a step beyond the pointer by rounding upwards.
            g.setColour(float(i)/20.f<=position?juce::Colour(0xff4cb2ff):juce::Colour(0xff27303b));
            const float dot=juce::jmax(2.f,d*.036f);
            g.fillEllipse(cx+std::sin(a)*r-dot*.5f,cy-std::cos(a)*r-dot*.5f,dot,dot);
        }
        const float cap=d*.87f;
        juce::Path ring;
        ring.addEllipse(cx-cap*.455f,cy-cap*.455f,cap*.91f,cap*.91f);
        ring.addEllipse(cx-cap*.329f,cy-cap*.329f,cap*.658f,cap*.658f);
        ring.setUsingNonZeroWinding(false);
        if(grip.isValid())
        {
            juce::Graphics::ScopedSaveState saved(g);
            g.reduceClipRegion(ring);
            // Rotate neutral rubber geometry, then light it in panel space.
            g.drawImageTransformed(grip,juce::AffineTransform::scale(cap/grip.getWidth(),cap/grip.getHeight())
                .followedBy(juce::AffineTransform::translation(-cap*.5f,-cap*.5f))
                .followedBy(juce::AffineTransform::rotation(angle))
                .followedBy(juce::AffineTransform::translation(cx,cy)));
            g.setGradientFill(juce::ColourGradient(juce::Colours::white.withAlpha(.08f),cx-cap*.4f,cy-cap*.4f,
                juce::Colours::black.withAlpha(.55f),cx+cap*.4f,cy+cap*.4f,false));
            g.fillPath(ring);
        }
        if(knob.isValid())
        {
            juce::Graphics::ScopedSaveState saved(g);
            juce::Path face;face.addEllipse(cx-cap*.337f,cy-cap*.337f,cap*.674f,cap*.674f);g.reduceClipRegion(face);
            g.drawImageTransformed(knob,juce::AffineTransform::scale(cap/knob.getWidth(),cap/knob.getHeight())
                .followedBy(juce::AffineTransform::translation(cx-cap*.5f,cy-cap*.5f)));
        }
        // The face and scene lighting stay fixed; grip and marker turn together.
        const float markerWidth=juce::jmax(1.5f,cap*.021f);
        juce::Path marker;
        marker.addRoundedRectangle(-markerWidth*.5f,-cap*.32f,markerWidth,cap*.14f,markerWidth*.5f);
        g.setColour(juce::Colour(0xfff4f5f7));
        g.fillPath(marker,juce::AffineTransform::rotation(angle).translated(cx,cy));
    }
    void drawLinearSlider(juce::Graphics& g,int x,int y,int w,int h,float pos,float,float,juce::Slider::SliderStyle style,juce::Slider& s) override
    {
        if(style!=juce::Slider::LinearVertical){LookAndFeel_V4::drawLinearSlider(g,x,y,w,h,pos,0,0,style,s);return;}
        const float cx=x+w*.5f;
        g.setColour(ink.withAlpha(.4f));
        for(int i=0;i<=10;++i){float yy=y+h*i/10.f;g.drawLine(cx-w*.32f,yy,cx+w*.32f,yy,1.f);}
        g.setColour(juce::Colour(0xff0b0d10));g.fillRoundedRectangle(cx-3,float(y),6,float(h),2);
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff5b6068),cx,pos-8,juce::Colour(0xff11151b),cx,pos+9,false));
        g.fillRoundedRectangle(cx-18,pos-8,36,16,5);
        g.setColour(light.withAlpha(.8f));g.drawLine(cx-12,pos-5,cx+12,pos-5,1.5f);
    }
    void drawButtonBackground(juce::Graphics& g,juce::Button& b,const juce::Colour&,bool hover,bool down) override
    {
        auto bounds=b.getLocalBounds().toFloat().reduced(1);
        const bool on=b.getToggleState();
        if(bool(b.getProperties()["partTab"])){
            juce::Path tab;tab.addRoundedRectangle(bounds.getX(),bounds.getY(),bounds.getWidth(),bounds.getHeight(),5,5,true,true,false,false);
            g.setGradientFill(juce::ColourGradient(on?juce::Colour(0xffe8ebef):juce::Colour(0xff666d78),0,bounds.getY(),on?juce::Colour(0xffbec3cb):juce::Colour(0xff343a43),0,bounds.getBottom(),false));g.fillPath(tab);g.setColour(juce::Colour(0xff243041));g.strokePath(tab,juce::PathStrokeType(1));if(on){g.setColour(blue);g.fillRect(bounds.removeFromBottom(3));}return;
        }
        if(bool(b.getProperties()["led"]))
        {
            g.setColour(on?juce::Colour(0xffb21718):juce::Colour(0xff26292d));g.fillEllipse(4,bounds.getCentreY()-6,12,12);
            g.setColour(on?juce::Colour(0xffff5450):juce::Colour(0xff6b7076));g.fillEllipse(7,bounds.getCentreY()-4,5,3);
            if(hover){g.setColour(blue.withAlpha(.16f));g.fillRoundedRectangle(bounds,3);}
            return;
        }
        const auto top=on?juce::Colour(0xff1695ee):juce::Colour(0xff434a54);
        const auto bottom=on?juce::Colour(0xff0752a7):juce::Colour(0xff171e28);
        g.setGradientFill(juce::ColourGradient(hover?top.brighter(.12f):top,0,bounds.getY(),down?bottom.darker(.2f):bottom,0,bounds.getBottom(),false));
        g.fillRoundedRectangle(bounds,4);g.setColour(juce::Colour(0xff10151c));g.drawRoundedRectangle(bounds,4,1);
        g.setColour(light.withAlpha(.22f));g.drawLine(bounds.getX()+3,bounds.getY()+2,bounds.getRight()-3,bounds.getY()+2);
    }
    void drawButtonText(juce::Graphics& g,juce::TextButton& b,bool,bool) override
    {
        g.setColour(b.isEnabled()?((bool(b.getProperties()["partTab"])&&b.getToggleState())?ink:(bool(b.getProperties()["led"])&&!bool(b.getProperties()["dark"])?ink:light)):light.withAlpha(.4f));
        g.setFont(juce::Font(bool(b.getProperties()["small"])?11.f:13.f,juce::Font::bold));
        auto area=b.getLocalBounds().reduced(3);
        if(bool(b.getProperties()["led"]))area.removeFromLeft(23);
        g.drawFittedText(b.getButtonText(),area,juce::Justification::centred,1);
    }
    void drawComboBox(juce::Graphics& g,int w,int h,bool,int,int,int,int,juce::ComboBox&) override
    {
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff344150),0,0,juce::Colour(0xff1a242e),0,float(h),false));
        g.fillRoundedRectangle(0,0,float(w),float(h),3);
        g.setColour(juce::Colour(0xff657180));g.drawRoundedRectangle(.5f,.5f,float(w)-1,float(h)-1,3,1);
        g.setColour(light);juce::Path arrow;arrow.addTriangle(float(w-15),h*.43f,float(w-7),h*.43f,float(w-11),h*.62f);g.fillPath(arrow);
    }
    juce::Font getComboBoxFont(juce::ComboBox&) override{return juce::Font(12.5f);}
};
struct ControlSlider final : juce::Slider
{
    bool wheelGesture=false;
    void mouseWheelMove(const juce::MouseEvent& event,const juce::MouseWheelDetails& wheel) override
    {
        const juce::ScopedValueSetter<bool> guard(wheelGesture,true);
        juce::Slider::mouseWheelMove(event,wheel);
    }
};
struct LfoIndicator final : juce::Label
{
    void paint(juce::Graphics& g) override
    {
        const bool on=bool(getProperties()["selected"]);
        const float y=getHeight()*.5f-6;
        g.setColour(on?juce::Colour(0xffb21718):juce::Colour(0xff26292d));g.fillEllipse(0,y,12,12);
        g.setColour(on?juce::Colour(0xffff5450):juce::Colour(0xff6b7076));g.fillEllipse(3,y+2,5,3);
        juce::Label::paint(g);
    }
};
struct Control final : juce::Component
{
    juce::RangedAudioParameter& parameter;
    const Spec& spec;
    juce::Label label;
    ControlSlider slider;
    juce::ComboBox menu;
    juce::TextButton button;
    juce::String kind;
    bool gesture=false;
    std::function<void(const juce::String&,bool)> report;
    Control(juce::RangedAudioParameter& p,const Spec& s,juce::String caption,juce::String type,bool isDark,
            std::function<void(const juce::String&,bool)> changed):parameter(p),spec(s),kind(type),report(std::move(changed))
    {
        setComponentID(juce::String("control:")+s.id);
        addAndMakeVisible(label);label.setText(caption,juce::dontSendNotification);
        label.setFont(juce::Font(12.5f));label.setColour(juce::Label::textColourId,isDark?light:ink);
        label.setJustificationType(juce::Justification::centred);
        setTooltipForControls(juce::String(s.group)+" / "+s.section+" / "+s.label);
        if(kind=="led")
        {
            label.setVisible(false);addAndMakeVisible(button);button.setComponentID(s.id);button.setButtonText(caption);
            button.getProperties().set("led",true);button.getProperties().set("dark",isDark);button.setClickingTogglesState(true);
            button.onClick=[this]{parameter.beginChangeGesture();setValue(button.getToggleState()?1:0);parameter.endChangeGesture();notify();};
        }
        else if(s.enumCount)
        {
            addAndMakeVisible(menu);menu.setComponentID(s.id);
            for(int i=0;i<s.enumCount;++i)menu.addItem(s.enumLabels[i],i+1);
            menu.onChange=[this]{parameter.beginChangeGesture();setValue(menu.getSelectedId()-1);parameter.endChangeGesture();notify();};
        }
        else
        {
            addAndMakeVisible(slider);slider.setComponentID(s.id);slider.setRange(s.minimum,s.maximum,1);
            const bool row=kind=="row";
            slider.setSliderStyle(row?juce::Slider::LinearHorizontal:(kind=="fader"?juce::Slider::LinearVertical:juce::Slider::RotaryVerticalDrag));
            slider.setRotaryParameters(juce::MathConstants<float>::pi*4.f/3.f,juce::MathConstants<float>::pi*8.f/3.f,true);
            slider.setTextBoxStyle(row?juce::Slider::TextBoxRight:juce::Slider::TextBoxBelow,false,row?56:64,row?24:18);
            if(kind=="mixer")slider.setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);
            slider.setColour(juce::Slider::textBoxTextColourId,isDark?light:ink);
            slider.setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);
            slider.setColour(juce::Slider::textBoxBackgroundColourId,juce::Colours::transparentBlack);
            slider.setColour(juce::Slider::trackColourId,blue);
            slider.setDoubleClickReturnValue(true,s.defaultValue);slider.setPopupDisplayEnabled(false,false,nullptr);
            slider.textFromValueFunction=[this](double v){return juce::String(juce::roundToInt(v)+spec.displayOffset);};
            slider.valueFromTextFunction=[this](const juce::String& t){return t.getDoubleValue()-spec.displayOffset;};
            slider.onDragStart=[this]{gesture=true;parameter.beginChangeGesture();notify();};
            slider.onDragEnd=[this]{parameter.endChangeGesture();gesture=false;if(slider.wheelGesture)notify();else report({},false);};
            slider.onValueChange=[this]
            {
                if(!gesture)parameter.beginChangeGesture();
                setValue(juce::roundToInt(slider.getValue()));
                if(!gesture)parameter.endChangeGesture();
                notify();
            };
        }
        refresh();
        addMouseListener(this,true);
    }
    ~Control() override{if(gesture)parameter.endChangeGesture();}
    void setValue(int v){parameter.setValueNotifyingHost(parameter.convertTo0to1(float(v)));}
    void mouseExit(const juce::MouseEvent&) override{if(!gesture)report({},false);}
    void setTooltipForControls(const juce::String& t){slider.setTooltip(t);menu.setTooltip(t);button.setTooltip(t);}
    void notify()
    {
        const auto v=int(std::round(parameter.convertFrom0to1(parameter.getValue())));
        report(juce::String(spec.label)+"\n"+(spec.enumCount?juce::String(spec.enumLabels[juce::jlimit(0,int(spec.enumCount)-1,v)]):juce::String(v+spec.displayOffset)),gesture);
    }
    void refresh()
    {
        const auto v=int(std::round(parameter.convertFrom0to1(parameter.getValue())));
        if(kind=="led")button.setToggleState(v!=0,juce::dontSendNotification);
        else if(spec.enumCount)menu.setSelectedId(v+1,juce::dontSendNotification);
        else if(!slider.isMouseButtonDown())
        {
            bool editing=false;
            for(int i=0;i<slider.getNumChildComponents();++i)
                if(auto* field=dynamic_cast<juce::Label*>(slider.getChildComponent(i)))editing|=field->isBeingEdited();
            if(!editing){slider.setValue(v,juce::dontSendNotification);slider.updateText();}
        }
    }
    void resized() override
    {
        auto area=getLocalBounds();
        if(kind=="led"){button.setBounds(area);return;}
        if(kind=="row")
        {
            label.setJustificationType(juce::Justification::centredLeft);label.setBounds(area.removeFromLeft(166));
            slider.setBounds(area);menu.setBounds(area.reduced(0,5));return;
        }
        if(kind=="mixer")
        {
            label.setJustificationType(juce::Justification::centredLeft);label.setBounds(area.removeFromLeft(51));slider.setBounds(area);return;
        }
        if(kind=="compact")
        {
            label.setFont(juce::Font(11.f));label.setBounds(area.removeFromLeft(39));slider.setBounds(area);return;
        }
        label.setBounds(area.removeFromTop(22));
        if(spec.enumCount)menu.setBounds(area.removeFromTop(juce::jmin(28,area.getHeight())).reduced(2,0));
        else slider.setBounds(area);
    }
};
struct Panel final : juce::Component
{
    juce::String title;
    std::vector<std::unique_ptr<Control>> controls;
    int rowHeight=40;
    void paint(juce::Graphics& g) override
    {
        g.setColour(ink.withAlpha(.1f));g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(.5f),4,1);
        g.setColour(ink);g.setFont(juce::Font(14.f,juce::Font::bold));g.drawText(title,12,6,getWidth()-24,24,juce::Justification::centredLeft);
    }
    void resized() override{int y=34;for(auto& c:controls){c->setBounds(12,y,getWidth()-24,rowHeight);y+=rowHeight;}}
};
}

struct Editor::Surface final : juce::Component, private juce::Timer
{
    Processor& processor;
    juce::var skin;
    SilverLook look;
    juce::TooltipWindow tooltip{this,750};
    juce::Component main, content;
    juce::Viewport viewport;
    juce::Label lcd,status,latency,help;
    juce::TextButton loadRom{"Load firmware"},folder{"ROM folder"},loadProgram{"Load program"},panic{"Panic"};
    juce::ComboBox mode,group,section,channel;
    std::array<juce::TextButton,5> pages;
    std::array<juce::TextButton,4> parts;
    juce::ComboBox firmwareSelect,skinSelect;
    juce::TextButton waveRom{"Wave ROM"},factory{"KS factory data"},customSkin{"Load skin JSON"};
    juce::ToggleButton performanceMode{"Performance (4 parts)"},partActive{"Part active"};
    Browser browser;
    bool displayedK=false,displayedPerformance=false;
    unsigned displayedPart=0;
    std::array<juce::TextButton,3> oscillators;
    juce::TextButton lfoSelect{"LFO SELECT"};
    std::array<LfoIndicator,2> lfoIndicators;
    std::vector<std::unique_ptr<juce::TextButton>> effects;
    juce::ComboBox pw;
    std::vector<std::unique_ptr<Control>> mainControls,effectControls;
    std::vector<std::unique_ptr<Panel>> panels;
    std::unique_ptr<juce::FileChooser> chooser;
    int page=0,osc=1,lfo=1,effect=0;
    bool displayHeld=false;
    double displayUntil=0;
    explicit Surface(Processor& p):processor(p),skin(skinData(p)),browser(p)
    {
        setLookAndFeel(&look);
        main.setComponentID("mainPanel");
        for(juce::Component* c:std::array<juce::Component*,12>{&main,&viewport,&lcd,&status,&latency,&loadRom,&folder,&loadProgram,&panic,&mode,&group,&section})addAndMakeVisible(c);
        addAndMakeVisible(channel);addAndMakeVisible(help);
        viewport.setViewedComponent(&content,false);viewport.setScrollBarsShown(true,false);viewport.setComponentID("controlViewport");
        for(auto* c:std::initializer_list<juce::Component*>{&firmwareSelect,&skinSelect,&waveRom,&factory,&customSkin,&performanceMode,&partActive,&browser})addAndMakeVisible(*c);
        firmwareSelect.setBounds(300,28,240,34);firmwareSelect.setComponentID("firmwareSelect");refreshFirmwares();
        firmwareSelect.onChange=[this]{int n=firmwareSelect.getSelectedId()-1;if(n<0 || n>=int(processor.firmwares.size()))return;juce::String error;if(!processor.loadFirmware(processor.firmwares[n].file,error)){status.setText(error,juce::dontSendNotification);showPage(4);}else{refreshModel();showPage(0);}};
        for(int i=0;i<4;++i){auto& b=parts[i];addAndMakeVisible(b);b.setButtonText(juce::String(i+1));b.getProperties().set("partTab",true);b.setComponentID("partTab"+juce::String(i+1));b.setBounds(555+i*62,28,56,34);b.onClick=[this,i]{processor.selectPart(unsigned(i));refreshModel();};}
        performanceMode.setBounds(52,96,250,34);performanceMode.onClick=[this]{juce::String error;if(!processor.setPerformanceMode(performanceMode.getToggleState(),error))status.setText(error,juce::dontSendNotification);};
        partActive.setBounds(330,96,200,34);partActive.onClick=[this]{juce::String error;processor.setPartActive(processor.currentPart(),partActive.getToggleState(),error);};
        browser.setBounds(24,86,1232,642);
        skinSelect.setBounds(52,353,230,34);for(const auto& name:juce::StringArray{"Auto","Silver-K","KS Grey","Classic","Custom"})skinSelect.addItem(name,skinSelect.getNumItems()+1);skinSelect.setText(processor.skinChoice,juce::dontSendNotification);skinSelect.onChange=[this]{processor.skinChoice=skinSelect.getText();skin=skinData(processor);rebuildMain();rebuildAdvanced();repaint();};
        customSkin.setBounds(298,353,190,34);customSkin.onClick=[this]{chooseAsset(3);};
        waveRom.setBounds(52,295,180,38);waveRom.onClick=[this]{chooseAsset(1);};factory.setBounds(246,295,190,38);factory.onClick=[this]{chooseAsset(2);};
        auto pageRect=juce::Rectangle<int>(850,28,74,34);
        for(int i=0;i<5;++i)
        {
            auto& b=pages[size_t(i)];addAndMakeVisible(b);b.setButtonText(juce::StringArray{"Main","Extra","Perf","Patches","Setup"}[i]);
            b.setComponentID("page"+b.getButtonText());b.setBounds(pageRect.translated(i*(pageRect.getWidth()+6),0));
            b.onClick=[this,i]{showPage(i);};
        }
        auto selections=skin["selectors"];
        auto oscillatorRect=rect(selections["osc"]),lfoRect=rect(selections["lfo"]);
        for(int i=0;i<3;++i)
        {
            auto& b=oscillators[size_t(i)];main.addAndMakeVisible(b);b.setButtonText("OSC "+juce::String(i+1));b.setComponentID("oscSelect"+juce::String(i+1));
            b.getProperties().set("led",true);b.setBounds(oscillatorRect.getX()+i*oscillatorRect.getWidth()/3,oscillatorRect.getY(),oscillatorRect.getWidth()/3,oscillatorRect.getHeight());
            b.onClick=[this,i]{osc=i+1;rebuildMain();};
        }
        main.addAndMakeVisible(lfoSelect);lfoSelect.setComponentID("lfoSelect");lfoSelect.setBounds(lfoRect);
        lfoSelect.onClick=[this]{lfo=lfo==1?2:1;rebuildMain();};
        for(int i=0;i<2;++i)
        {
            auto& indicator=lfoIndicators[size_t(i)];main.addAndMakeVisible(indicator);
            indicator.setComponentID("lfoIndicator"+juce::String(i+1));indicator.setText(juce::String(i+1),juce::dontSendNotification);
            indicator.setColour(juce::Label::textColourId,ink);indicator.setFont(juce::Font(12.f));indicator.setJustificationType(juce::Justification::centredRight);
            indicator.setInterceptsMouseClicks(false,false);indicator.setBounds(rect(selections["lfoIndicators"][i]));
        }
        main.addAndMakeVisible(pw);pw.setBounds(rect(selections["pw"]));pw.setComponentID("pwSource");
        pw.addItem("PW position",1);pw.addItem("LFO 2 PWM",2);pw.addItem("Env PWM",3);pw.setSelectedId(1,juce::dontSendNotification);
        pw.setTooltip("Choose which existing pulse-width control to edit. All three retain their values.");pw.onChange=[this]{rebuildMain();};
        const auto fxRect=rect(selections["fx"]);const auto* fxArray=skin["effects"].getArray();
        for(int i=0;i<fxArray->size();++i)
        {
            auto b=std::make_unique<juce::TextButton>((*fxArray)[i]["name"].toString());main.addAndMakeVisible(*b);
            b->getProperties().set("small",true);b->setComponentID("effect"+(*fxArray)[i]["name"].toString());
            b->setBounds(fxRect.getX()+i*fxRect.getWidth()/fxArray->size(),fxRect.getY(),fxRect.getWidth()/fxArray->size()-2,fxRect.getHeight());
            b->setTooltip("Edit "+b->getButtonText()+". Other effects keep processing.");b->onClick=[this,i]{effect=i;rebuildEffects();};effects.push_back(std::move(b));
        }
        lcd.setColour(juce::Label::backgroundColourId,juce::Colour(0xff0752ba));lcd.setColour(juce::Label::textColourId,light);
        lcd.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(),16.f,juce::Font::plain));lcd.setJustificationType(juce::Justification::centredLeft);
        lcd.setComponentID("programDisplay");showProgram();lcd.setBorderSize(juce::BorderSize<int>(8));lcd.setBounds(rect(skin["lcd"]));
        loadProgram.setBounds(rect(skin["actions"]["loadProgram"]));panic.setBounds(rect(skin["actions"]["panic"]));
        loadRom.onClick=[this]{selectFile(true);};folder.onClick=[this]{chooseAsset(0);};
        loadProgram.setButtonText("Patch browser");loadProgram.onClick=[this]{showPage(3);};panic.onClick=[this]{for(uint8_t c=0;c<16;++c){processor.sendShortMidi(0xb0|c,64,0);processor.sendShortMidi(0xb0|c,123,0);processor.sendShortMidi(0xb0|c,120,0);}showValue("Panic\nAll notes off",false);};
        mode.setComponentID("controlMode");group.setComponentID("controlGroup");section.setComponentID("controlSection");
        mode.addItem("All sections",1);mode.addItem("Single section",2);mode.setSelectedId(1,juce::dontSendNotification);
        juce::StringArray groups;for(const auto& s:processor.visibleControls())groups.addIfNotAlreadyThere(s.group);
        for(int i=0;i<groups.size();++i)group.addItem(groups[i],i+1);group.setSelectedId(2,juce::dontSendNotification);
        mode.onChange=[this]{rebuildSections();};group.onChange=[this]{rebuildSections();};section.onChange=[this]{rebuildAdvanced();};
        auto advanced=skin["advanced"];mode.setBounds(rect(advanced["mode"]));group.setBounds(rect(advanced["group"]));section.setBounds(rect(advanced["section"]));viewport.setBounds(rect(advanced["viewport"]));
        auto setup=skin["setup"];status.setBounds(rect(setup["status"]));status.setFont(juce::Font(17.f));status.setColour(juce::Label::textColourId,ink);
        status.setText(processor.status(),juce::dontSendNotification);latency.setBounds(rect(setup["latency"]));latency.setColour(juce::Label::textColourId,ink);
        loadRom.setBounds(rect(setup["firmware"]));folder.setBounds(rect(setup["folder"]));channel.setBounds(rect(setup["channel"]));channel.setComponentID("controlChannel");
        for(int i=1;i<=16;++i)channel.addItem("Editor MIDI channel "+juce::String(i),i);channel.setSelectedId(1,juce::dontSendNotification);channel.onChange=[this]{processor.setEditorChannel(uint8_t(channel.getSelectedId()-1));};
        help.setBounds(52,444,1120,84);help.setColour(juce::Label::textColourId,ink);help.setFont(juce::Font(15.f));
        help.setText("ROM folder selects the supported firmware. KS needs factory MIDI or user Flash.\nWithout a wave ROM, sampled oscillators are silent. The zero bank adapter is disabled for a real ROM.\nPatch folders, model, tags, favourites and compatibility are available on Patches.",juce::dontSendNotification);
        processor.refreshControls();main.setBounds(0,0,int(skin["width"]),int(skin["height"]));rebuildMain();rebuildSections();showPage(processor.firmwareReady()?0:4);timerCallback();startTimerHz(20);
    }
    ~Surface() override{stopTimer();viewport.setViewedComponent(nullptr,false);setLookAndFeel(nullptr);}
    void showProgram()
    {
        displayHeld=false;displayUntil=0;
        auto name=processor.currentName();
        if(name.isEmpty()||name=="default")name="Init Program";
        lcd.setText("Preset\n"+name,juce::dontSendNotification);
    }
    void showValue(const juce::String& message,bool held)
    {
        if(message.isEmpty()){showProgram();return;}
        displayHeld=held;displayUntil=juce::Time::getMillisecondCounterHiRes()+900;
        lcd.setText(message,juce::dontSendNotification);
    }
    void addControl(std::vector<std::unique_ptr<Control>>& list,juce::Component& parent,juce::String id,juce::String caption,juce::String kind,bool isDark,juce::Rectangle<int> bounds)
    {
        const auto* s=processor.spec(id);if(!s)return;
        if(auto* parameter=processor.uiParameter(s->id))
        {
            auto c=std::make_unique<Control>(*parameter,*s,caption.isEmpty()?s->label:caption,kind,isDark && !greyLower(),[this](const juce::String& message,bool held){showValue(message,held);});
            // Text boxes are created before attachment; rebuild them with the
            // skin's editable-label styling, including its transparent frame.
            c->setLookAndFeel(&look);
            c->slider.getProperties().set("rotaryStyle",kind=="classic"?"classic":"led");
            c->slider.getProperties().set("rotaryDiameter",skin[kind=="classic"?"classicDiameter":(id=="cutoff"?"cutoffDiameter":"knobDiameter")]);
            parent.addAndMakeVisible(*c);c->setBounds(bounds);list.push_back(std::move(c));
        }
    }
    void rebuildMain()
    {
        showProgram();
        mainControls.clear();
        const juce::String sources[]{"pulse_width","lfo2_pwm","env_pwm"};
        for(auto item:*skin["controls"].getArray())
        {
            auto id=item["id"].toString().replace("{osc}",juce::String(osc)).replace("{lfo}",juce::String(lfo));
            auto caption=item["label"].toString();
            if(id.contains("{pw}")){id=id.replace("{pw}",sources[juce::jmax(0,pw.getSelectedId()-1)]);caption=pw.getText();}
            addControl(mainControls,main,id,caption,item["kind"].toString(),bool(item["dark"]),rect(item["rect"]));
        }
        for(int i=0;i<3;++i)oscillators[size_t(i)].setToggleState(osc==i+1,juce::dontSendNotification);
        lfoSelect.setTooltip("Currently editing LFO "+juce::String(lfo)+". Click to select LFO "+juce::String(lfo==1?2:1)+".");
        for(int i=0;i<2;++i)
        {
            auto& indicator=lfoIndicators[size_t(i)];indicator.getProperties().set("selected",lfo==i+1);
            indicator.setTitle("LFO "+juce::String(i+1)+(lfo==i+1?" selected":" inactive"));indicator.repaint();
        }
        if(!processor.isKStation()){
          addControl(mainControls,main,"filter_type","Type","menu",false,{522,466,140,44});
          for(auto& c:mainControls)if(juce::String(c->spec.id)=="filter_slope")c->setBounds(680,466,145,44);
        }
        rebuildEffects();main.setEnabled(processor.firmwareReady());
    }
    void rebuildEffects()
    {
        showProgram();
        effectControls.clear();auto config=skin["effects"][effect];int i=0;
        for(auto id:*config["controls"].getArray())
        {addControl(effectControls,main,id.toString(),config["labels"][i].toString(),"classic",true,rect(skin["effectSlots"][i]));++i;}
        i=0;for(auto id:*config["menus"].getArray())
        {addControl(effectControls,main,id.toString(),{},"row",true,rect(skin["effectMenus"][i++]));}
        // Menus at the bottom are compact; their full parameter names are in tooltips.
        for(auto& c:effectControls)if(c->spec.enumCount){c->label.setVisible(false);c->menu.setBounds(c->getLocalBounds());}
        for(size_t n=0;n<effects.size();++n)effects[n]->setToggleState(int(n)==effect,juce::dontSendNotification);
    }
    void showPage(int selected)
    {
        showProgram();
        page=selected;
        for(size_t i=0;i<pages.size();++i)pages[i].setToggleState(int(i)==page,juce::dontSendNotification);
        const bool isMain=page==0,isAdvanced=page==1||page==2,isSetup=page==4;
        main.setVisible(isMain);lcd.setVisible(isMain);loadProgram.setVisible(isMain);panic.setVisible(isMain);
        viewport.setVisible(isAdvanced);mode.setVisible(isAdvanced);group.setVisible(page==1);section.setVisible(page==1&&mode.getSelectedId()==2);mode.setVisible(page==1);browser.setVisible(page==3);performanceMode.setVisible(page==2);partActive.setVisible(page==2);
        viewport.setBounds(page==2?juce::Rectangle<int>(24,146,1232,582):rect(skin["advanced"]["viewport"]));
        for(juce::Component* c:std::array<juce::Component*,6>{&status,&latency,&loadRom,&folder,&channel,&help})c->setVisible(isSetup);
        waveRom.setVisible(isSetup);factory.setVisible(isSetup);skinSelect.setVisible(isSetup);customSkin.setVisible(isSetup);
        loadRom.setBounds(52,245,200,38);folder.setBounds(266,245,170,38);
        if(page==1)rebuildSections();if(page==2)rebuildAdvanced();
        repaint();
    }
    void rebuildSections()
    {
        const auto selected=section.getText();section.clear(juce::dontSendNotification);juce::StringArray names;
        for(const auto& s:processor.visibleControls())if(group.getText()==s.group)names.addIfNotAlreadyThere(s.section);
        for(int i=0;i<names.size();++i)section.addItem(names[i],i+1);
        section.setSelectedId(juce::jmax(0,names.indexOf(selected))+1,juce::dontSendNotification);
        section.setVisible(page==1&&mode.getSelectedId()==2);rebuildAdvanced();
    }
    void rebuildAdvanced()
    {
        panels.clear();
        const auto layout=skin["advanced"];const int rowHeight=int(layout["rowHeight"]),columnWidth=int(layout["columnWidth"]),gap=int(layout["gap"]);
        for(const auto& s:processor.visibleControls())
        {
            if(page==2){if(juce::String(s.group)!="Part")continue;}else if(juce::String(s.group)=="Part" || group.getText()!=s.group||(mode.getSelectedId()==2&&section.getText()!=s.section))continue;
            Panel* panel=nullptr;for(auto& p:panels)if(p->title==s.section){panel=p.get();break;}
            if(!panel){auto p=std::make_unique<Panel>();p->title=s.section;p->rowHeight=rowHeight;panel=p.get();content.addAndMakeVisible(*p);panels.push_back(std::move(p));}
            addControl(panel->controls,*panel,s.id,s.label,"row",false,{});
        }
        std::array<int,3> ys{0,0,0};
        for(auto& p:panels){const auto col=size_t(std::min_element(ys.begin(),ys.end())-ys.begin());const int h=42+rowHeight*int(p->controls.size());p->setBounds(int(col)*(columnWidth+gap),ys[col],columnWidth,h);ys[col]+=h+gap;}
        content.setSize(viewport.getWidth()-16,juce::jmax(viewport.getHeight(),*std::max_element(ys.begin(),ys.end())));viewport.setViewPosition(0,0);content.setEnabled(processor.firmwareReady());
    }
    void paint(juce::Graphics& g) override
    {
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xffeef0f2),0,0,processor.skinChoice=="Classic"?juce::Colour(0xffd9d4c6):silver,0,float(getHeight()),false));g.fillAll();
        const auto header=rect(skin["header"]).toFloat();g.setGradientFill(juce::ColourGradient(processor.skinChoice=="Classic"?juce::Colour(0xff438276):juce::Colour(0xff148de3),0,header.getY(),processor.skinChoice=="Classic"?juce::Colour(0xff1c5048):juce::Colour(0xff075293),0,header.getBottom(),false));g.fillRoundedRectangle(header,3);
        g.setColour(light);g.setFont(juce::Font(36.f,juce::Font::bold));g.drawText(skin["name"].toString(),rect(juce::var{juce::Array<juce::var>{40,21,250,45}}),juce::Justification::centredLeft);
        if(page==0)
        {
            const int split=int(skin["split"]);g.setGradientFill(juce::ColourGradient(greyLower()?juce::Colour(0xffc4c7cc):juce::Colour(0xff303237),0,float(split),greyLower()?juce::Colour(0xffaeb2b9):juce::Colour(0xff1d2025),0,float(getHeight()),false));g.fillRect(18,split,getWidth()-36,getHeight()-split-16);
            for(auto sectionData:*skin["sections"].getArray())
            {
                const auto* a=sectionData.getArray();const int x=int((*a)[0]),y=int((*a)[1]),w=int((*a)[2]),h=int((*a)[3]);
                const bool low=y>=split;g.setColour(low&&!greyLower()?light:ink);g.setFont(juce::Font(16.f,juce::Font::bold));g.drawText((*a)[4].toString(),x+5,y+4,w-10,24,juce::Justification::centredLeft);
                g.setColour((low&&!greyLower()?light:ink).withAlpha(.35f));g.drawLine(float(x+w+6),float(y),float(x+w+6),float(y+h),1);
            }
            g.setColour(juce::Colour(0xff0b1320));g.fillRoundedRectangle(rect(skin["lcd"]).toFloat().expanded(5),3);
        }
        else if(page==4)
        {
            g.setColour(ink);g.setFont(juce::Font(22.f,juce::Font::bold));g.drawText("Instrument setup",52,94,900,34,juce::Justification::centredLeft);
        }
        g.setColour(ink.withAlpha(.4f));g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1),5,1);
    }
    void timerCallback() override
    {
        if(!displayHeld&&juce::Time::getMillisecondCounterHiRes()>=displayUntil)showProgram();
        processor.refreshControls();
        if(displayedK!=processor.isKStation() || displayedPart!=processor.currentPart() || displayedPerformance!=processor.performanceMode())refreshModel();
        performanceMode.setEnabled(!processor.isKStation() && processor.firmwareReady());performanceMode.setToggleState(processor.performanceMode(),juce::dontSendNotification);
        partActive.setEnabled(!processor.isKStation() && processor.performanceMode());partActive.setToggleState((processor.activeParts()&(1u<<processor.currentPart()))!=0,juce::dontSendNotification);
        for(unsigned i=0;i<4;++i){const bool available=i==0 || (!processor.isKStation() && processor.performanceMode());parts[i].setEnabled(available);parts[i].setAlpha(available?1.f:.3f);parts[i].setToggleState(i==processor.currentPart(),juce::dontSendNotification);}
        for(auto& c:mainControls)c->refresh();for(auto& c:effectControls)c->refresh();for(auto& p:panels)for(auto& c:p->controls)c->refresh();
        const bool ready=processor.firmwareReady();main.setEnabled(ready);content.setEnabled(ready);loadProgram.setEnabled(true);
        const auto rate=processor.getSampleRate();latency.setText(rate>0?"Host latency: "+juce::String(1000.*processor.getLatencySamples()/rate,1)+" ms":"",juce::dontSendNotification);
    }
    bool greyLower()const{return processor.skinChoice=="KS Grey" || (processor.skinChoice=="Auto"&&!processor.isKStation());}
    void refreshFirmwares(){firmwareSelect.clear(juce::dontSendNotification);for(size_t i=0;i<processor.firmwares.size();++i){const auto& f=processor.firmwares[i];firmwareSelect.addItem(juce::String(f.kStation?"K-Station: ":"KS: ")+f.file.getFileName(),int(i)+1);if(f.file==processor.osFile)firmwareSelect.setSelectedId(int(i)+1,juce::dontSendNotification);}if(!firmwareSelect.getSelectedId())firmwareSelect.setText(processor.osFile.existsAsFile()?juce::String(processor.isKStation()?"K-Station: ":"KS: ")+processor.osFile.getFileName():"Choose firmware",juce::dontSendNotification);}
    void refreshModel(){displayedK=processor.isKStation();displayedPart=processor.currentPart();displayedPerformance=processor.performanceMode();group.clear(juce::dontSendNotification);juce::StringArray names;for(const auto& c:processor.visibleControls())if(juce::String(c.group)!="Part")names.addIfNotAlreadyThere(c.group);for(int i=0;i<names.size();++i)group.addItem(names[i],i+1);group.setSelectedId(2,juce::dontSendNotification);status.setText(processor.status(),juce::dontSendNotification);rebuildMain();rebuildSections();refreshFirmwares();repaint();}
    void selectFile(bool){chooseAsset(4);}
    void chooseAsset(int role){const bool directory=role==0;chooser=std::make_unique<juce::FileChooser>(directory?"Select ROM folder":role==1?"Select KS wave ROM":role==2?"Select KS factory data":role==3?"Select skin JSON":"Load firmware",processor.romFolder,directory?"*":role==3?"*.json":role==2?"*.bin;*.mid;*.midi":"*.bin;*.syx;*.zip;*.os;*.OS");const juce::Component::SafePointer<Surface> self(this);chooser->launchAsync(juce::FileBrowserComponent::openMode|(directory?juce::FileBrowserComponent::canSelectDirectories:juce::FileBrowserComponent::canSelectFiles),[self,role](const juce::FileChooser& c){if(!self)return;auto file=c.getResult();if(file==juce::File{})return;juce::String error;bool ok=true;if(role==0)ok=self->processor.setRomFolder(file,error);else if(role==1){if(file.getSize()!=129024){ok=false;error="Wave ROM must contain exactly 129024 bytes";}else self->processor.bankFile=file;}else if(role==2)self->processor.flashFile=file;else if(role==3){auto data=juce::JSON::parse(file);if(!data.isObject() || !data["controls"].isArray() || !data["effects"].isArray() || int(data["width"])!=1280 || int(data["height"])!=752){ok=false;error="Expected a Silver-K 1280 x 752 JSON skin";}else{self->processor.customSkin=file.getFullPathName();self->processor.skinChoice="Custom";self->skin=skinData(self->processor);self->skinSelect.setText("Custom",juce::dontSendNotification);self->rebuildMain();self->rebuildAdvanced();self->repaint();}}else ok=self->processor.loadFirmware(file,error);
        if(ok&&(role==1||role==2)&&self->processor.osFile.existsAsFile()&&!self->processor.isKStation())ok=self->processor.loadFirmware(self->processor.osFile,error);
        self->status.setText(ok?self->processor.status():error,juce::dontSendNotification);self->refreshFirmwares();if(ok&&role==4){self->refreshModel();self->showPage(0);}else self->showPage(4);});}

};
Editor::Editor(Processor& p):AudioProcessorEditor(p),m_surface(std::make_unique<Surface>(p))
{
    const int width=int(m_surface->skin["width"]),height=int(m_surface->skin["height"]);
    addAndMakeVisible(*m_surface);setResizable(true,false);setResizeLimits(width*3/4,height*3/4,width*3/2,height*3/2);
    if(auto* c=getConstrainer())c->setFixedAspectRatio(double(int(m_surface->skin["width"]))/int(m_surface->skin["height"]));
    setSize(int(m_surface->skin["width"]),int(m_surface->skin["height"]));
}
Editor::~Editor()=default;
void Editor::resized()
{
    if(!m_surface)return;const int width=int(m_surface->skin["width"]),height=int(m_surface->skin["height"]);
    m_surface->setBounds(0,0,width,height);m_surface->setTransform(juce::AffineTransform::scale(float(getWidth())/width,float(getHeight())/height));
}
}
