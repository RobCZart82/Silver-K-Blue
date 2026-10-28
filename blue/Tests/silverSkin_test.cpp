#include "silverSkin.h"
#include "silverEditor.h"
#include <iostream>
#include <stdexcept>
using namespace silverK;
static void check(bool ok,const juce::String& msg){if(!ok)throw std::runtime_error(msg.toStdString());}
static juce::Component* find(juce::Component& c,const juce::String& id){if(c.getComponentID()==id)return &c;for(auto* child:c.getChildren())if(auto* f=find(*child,id))return f;return nullptr;}
static void click(juce::Component& c,const char* id){auto* b=dynamic_cast<juce::TextButton*>(find(c,id));check(b&&bool(b->onClick),juce::String("Missing action ")+id);b->onClick();}
static void capture(juce::Component& c,const juce::File& f){f.deleteFile();auto s=f.createOutputStream();check(s&&juce::PNGImageFormat().writeImageToStream(c.createComponentSnapshot(c.getLocalBounds(),true,2.f),*s),"Snapshot");}
static bool equal(const juce::Image& a,const juce::Image& b){if(a.getBounds()!=b.getBounds())return false;for(int y=0;y<a.getHeight();++y)for(int x=0;x<a.getWidth();++x)if(a.getPixelAt(x,y)!=b.getPixelAt(x,y))return false;return true;}
static juce::var object(){return juce::var(new juce::DynamicObject);}
static void set(juce::var& v,const char* k,juce::var x){v.getDynamicObject()->setProperty(k,x);}
static void save(const juce::File& f,const juce::var& v){check(f.replaceWithText(juce::JSON::toString(v)),"Save fixture");}
static void widgetTests(const SkinPackage& pack){
 SkinLookAndFeel look(pack);juce::Label label;label.setText("Preset\nArvizturo 123",juce::dontSendNotification);label.setBounds(0,0,278,100);
 juce::Image first(juce::Image::ARGB,556,200,true);{juce::Graphics g(first);g.addTransform(juce::AffineTransform::scale(2.f));check(look.drawLcdText(g,label,label.getText(),{8,8,262,84},2),"Latin matrix text");}
 juce::Image changed(juce::Image::ARGB,556,200,true);{juce::Graphics g(changed);g.addTransform(juce::AffineTransform::scale(2.f));check(look.drawLcdText(g,label,"Preset\nA different preset",{8,8,262,84},2),"Live matrix update");}
 check(!equal(first,changed),"Matrix does not track live text");check(label.getText()=="Preset\nArvizturo 123","Rendering changed label data");
 {juce::Graphics g(first);check(!look.drawLcdText(g,label,juce::String::fromUTF8("\xe6\x97\xa5\xe6\x9c\xac"),{8,8,262,84},2),"Unicode must fall back to native");check(look.drawLcdText(g,label,juce::String::fromUTF8("Árvíztűrő tükörfúrógép"),{8,8,262,84},2),"Hungarian glyph mapping");check(look.drawLcdText(g,label,juce::String::repeatedString("Long preset ",20),{8,8,262,84},2),"Long text bounds");}
 juce::ComboBox combo;combo.setSize(180,28);combo.addItem("Test",1);combo.setSelectedId(1);
 juce::Image bg(juce::Image::ARGB,180,28,true);{juce::Graphics g(bg);check(look.drawLcdComboBackground(g,combo),"LCD ComboBox background missing");}
 check(bg.getPixelAt(20,12)!=bg.getPixelAt(21,12),"No procedural grid texture");
 auto local=object(),lcd=object();set(lcd,"enabled",false);set(local,"lcd",lcd);set(local,"comboBoxLcd",lcd);label.getProperties().set("silverSkinStyle",local);combo.getProperties().set("silverSkinStyle",local);
 {juce::Graphics g(first);check(!look.drawLcdText(g,label,"Native",{8,8,262,84},2),"Local LCD off override ignored");check(!look.drawLcdComboBackground(g,combo),"Local combo off override ignored");}
 auto builtin=SkinPackage::builtin();SkinLookAndFeel old(builtin);juce::Label plain;
 {juce::Graphics g(first);check(!old.drawLcdText(g,plain,"Native",{8,8,262,84},2),"Old skins changed");check(!old.drawLcdComboBackground(g,combo),"Old combo changed");}
 std::cout<<"PASS: live LCD pixels, Unicode fallback, Hungarian mapping, long names, native defaults, local overrides and procedural combo grid\n";
}
static void lcdRevisionTests(const SkinPackage& pack,const juce::File& previews){
 const auto wrapped=SkinLookAndFeel::wrapLcdLines("Preset\nSPRUNG / PART 1: VIBRATINE",24);
 check(wrapped.size()==3 && wrapped[0]=="Preset" && wrapped[1]=="SPRUNG / PART 1:" && wrapped[2]=="VIBRATINE","Word wrap of the reported patch name");
 const auto word=juce::String::repeatedString("ABCDEFGHIJKLMNOPQRSTUVWXYZ",3);
 const auto hard=SkinLookAndFeel::wrapLcdLines(word,24);check(hard.joinIntoString("")==word,"Hard wrap lost characters");
 check(SkinLookAndFeel::wrapLcdLines("A\n\nB",24).size()==3,"Explicit blank line lost");
 const auto four=SkinLookAndFeel::wrapLcdLines("Preset\n"+juce::String::repeatedString("X",64),24);check(four.size()==4,"Four-row capacity incorrect");
 SkinLookAndFeel look(pack);juce::Label main;main.setComponentID("programDisplay");main.setSize(278,100);
 auto style=object();set(style,"lcd",pack.theme["components"]["programDisplay"]["lcd"]);main.getProperties().set("silverSkinStyle",style);
 auto draw=[&](const juce::String& text){main.setText(text,juce::dontSendNotification);juce::Image im(juce::Image::ARGB,556,200,true);juce::Graphics g(im);g.addTransform(juce::AffineTransform::scale(2.f));look.drawLabel(g,main);return im;};
 auto shortName=draw("Preset\nSHORT"),longName=draw("Preset\nSPRUNG / PART 1: VIBRATINE");
 // Same heading must have identical pixel geometry even when the preset name grows.
 for(int y=0;y<54;++y)for(int x=0;x<556;++x)check(shortName.getPixelAt(x,y)==longName.getPixelAt(x,y),"Long name shrank or moved the four-row heading");
 main.setText("Preset\n"+juce::String::repeatedString("X",64),juce::dontSendNotification);auto allFour=draw(main.getText());
 int fourthRow=0;for(int y=140;y<176;++y)for(int x=0;x<556;++x)if(allFour.getPixelAt(x,y).getAlpha()>128)++fourthRow;check(fourthRow>50,"Fourth LCD row was not painted");
 juce::ComboBox combo;combo.setLookAndFeel(&look);combo.setSize(200,30);combo.addItem("Triangle",1);combo.setSelectedId(1,juce::dontSendNotification);
 juce::Label* field=nullptr;for(auto* c:combo.getChildren())if(auto* label=dynamic_cast<juce::Label*>(c))field=label;check(field!=nullptr,"ComboBox text label missing");
 juce::Image actual(juce::Image::ARGB,field->getWidth(),field->getHeight(),true),expected=actual.createCopy();
 {juce::Graphics g(actual);look.drawLabel(g,*field);}{juce::Graphics g(expected);look.juce::LookAndFeel_V4::drawLabel(g,*field);}
 check(equal(actual,expected),"Normal dropdown not restored to native label rendering");check(combo.getText()=="Triangle"&&combo.getSelectedId()==1,"Font restoration changed selected value");
 auto performanceStyle=object();set(performanceStyle,"lcd",pack.theme["components"]["performance:part1:preset"]["lcd"]);combo.getProperties().set("performancePreset",true);combo.getProperties().set("silverSkinStyle",performanceStyle);
 juce::Image perf(juce::Image::ARGB,field->getWidth(),field->getHeight(),true);{juce::Graphics g(perf);look.drawLabel(g,*field);}check(equal(perf,expected),"Performance preset dropdown still renders matrix text");
 juce::PopupMenu::Item item("Triangle");item.itemID=1;item.isTicked=true;
 const auto options=juce::PopupMenu::Options().withTargetComponent(&combo);
 juce::Image menu(juce::Image::ARGB,240,28,true),native=menu.createCopy();
 {juce::Graphics g(menu);look.drawPopupMenuItemWithOptions(g,{0,0,240,28},false,item,options);}{juce::Graphics g(native);look.juce::LookAndFeel_V4::drawPopupMenuItemWithOptions(g,{0,0,240,28},false,item,options);}
 check(equal(menu,native),"Opened dropdown still renders matrix text");
 auto file=previews.getChildFile("native-dropdown-row.png");file.deleteFile();auto output=file.createOutputStream();check(output&&juce::PNGImageFormat().writeImageToStream(menu,*output),"Native menu preview");
 combo.setLookAndFeel(nullptr);
 std::cout<<"PASS: native fonts restored in normal/performance dropdowns and opened lists; fixed four-row main LCD and wrapping preserved\n";
}
static void navyUniformity(const SkinPackage& pack,const juce::File& root){
 const auto size=pack.name.contains("192")?"192":"256";
 const auto baseline=root.getSiblingFile("Silver-K-Skin-Features-R2-2026-10-25").getChildFile("Skins/Silver-K Features R2 Retina "+juce::String(size)+"/skin.json");
 SkinPackage original;juce::String error;check(SkinPackage::load(baseline,original,error),error);
 const auto a=pack.theme["comboBoxLcd"],b=original.theme["comboBoxLcd"];
 for(const auto* key:{"topColour","bottomColour","gridColour","borderColour","gridAlpha","gridPitch","dotSize","fitText","inactiveAlpha"})check(a[juce::Identifier(key)]==b[juce::Identifier(key)],"R2 background / geometry changed: "+juce::String(key));
 check(juce::JSON::toString(pack.theme["lcd"])==juce::JSON::toString(original.theme["lcd"]),"Main/Performance LCD style changed");
 SkinLookAndFeel look(pack),before(original);juce::ComboBox firmware;firmware.setComponentID("firmwareSelect");firmware.setSize(240,80);auto style=object();set(style,"comboBoxLcd",pack.theme["components"]["firmwareSelect"]["comboBoxLcd"]);firmware.getProperties().set("silverSkinStyle",style);
 auto closed=[&](SkinLookAndFeel& l){juce::Image out(juce::Image::ARGB,240,80,true);juce::Graphics g(out);check(l.drawLcdComboBackground(g,firmware),"Firmware grid missing");return out;};
 check(equal(closed(look),closed(before)),"Closed firmware background differs from R2");
 juce::Image menu(juce::Image::ARGB,240,80,true);{juce::Graphics g(menu);look.drawPopupMenuBackgroundWithOptions(g,240,80,juce::PopupMenu::Options().withTargetComponent(&firmware));}const auto face=closed(look);for(int y=2;y<78;++y)for(int x=2;x<238;++x)check(menu.getPixelAt(x,y)==face.getPixelAt(x,y),"Opened dropdown does not share R2 navy background");
 juce::Image context(juce::Image::ARGB,240,80,true);{juce::Graphics g(context);look.drawPopupMenuBackgroundWithOptions(g,240,80,{});}check(equal(context,menu),"Another popup uses a different background");
 check(!a.hasProperty("cellGridAlpha")&&!a.hasProperty("gridLineWidth")&&!a.hasProperty("pixelFill")&&!a.hasProperty("disabledAlpha"),"R3 presentation leaked into R2");
 std::cout<<"PASS: closed firmware background unchanged from R2; all popup backgrounds match it; no R3 contrast or opacity changes\n";
}
static void validation(const juce::File& root){
 juce::File manifest;juce::String error;check(SkinPackage::exportTemplate(root,manifest,error),error);const auto f=manifest.getSiblingFile("theme.json");const auto original=juce::JSON::parse(f);
 for(const auto* field:{"gridPitch","columns","enabled","rows","wrap","dotMatrixText"}){
  auto theme=original.clone(),cfg=object();set(cfg,field,(juce::String(field)=="enabled"||juce::String(field)=="wrap"||juce::String(field)=="dotMatrixText")?juce::var("yes"):juce::var(0));set(theme,(juce::String(field)=="columns"||juce::String(field)=="rows"||juce::String(field)=="wrap")?"lcd":"comboBoxLcd",cfg);save(f,theme);SkinPackage candidate;check(!SkinPackage::load(manifest,candidate,error),juce::String("Invalid config accepted: ")+field);
 }
 save(f,original);SkinPackage good;check(SkinPackage::load(manifest,good,error),error);
 std::cout<<"PASS: invalid grid pitch / column count / enabled flag rejected; legacy package valid\n";
}
static void waveform(juce::Component& c){
 int count=0;
 for(int osc=1;osc<=3;++osc)for(int n=0;n<4;++n){
  auto* b=dynamic_cast<juce::TextButton*>(find(c,"osc"+juce::String(osc)+"_wave:"+juce::String(n)));check(b!=nullptr,"K waveform button missing");const bool originalToggle=b->getToggleState();b->setEnabled(true);b->setToggleState(true,juce::dontSendNotification);
  auto paint=[&](bool hover){juce::Image im(juce::Image::ARGB,b->getWidth(),b->getHeight(),true);juce::Graphics g(im);b->getLookAndFeel().drawButtonBackground(g,*b,juce::Colours::transparentBlack,hover,false);return im;};
  auto normal=paint(false),hover=paint(true);check(normal.getPixelAt(2,2).getAlpha()==0,"Selected waveform retained a permanent background");check(hover.getPixelAt(2,2).getAlpha()>0,"Hover background missing behind custom artwork");
  b->setEnabled(false);check(paint(true).getPixelAt(2,2).getAlpha()==0,"Disabled waveform received hover");b->setEnabled(true);b->setToggleState(originalToggle,juce::dontSendNotification);++count;
 }
 check(count==12,"Waveform count");std::cout<<"PASS: all 12 K waveform buttons: clean selected state, hover behind artwork, disabled state\n";
}
static juce::TooltipWindow* tooltipWindow(juce::Component& c){if(auto* tip=dynamic_cast<juce::TooltipWindow*>(&c))return tip;for(auto* child:c.getChildren())if(auto* tip=tooltipWindow(*child))return tip;return nullptr;}
static int tooltipChecks(const juce::File& output,const juce::File& manifest){
 output.createDirectory();juce::String error;SkinPackage package;check(SkinPackage::load(manifest,package,error),error);SkinLookAndFeel look(package);
 for(const auto& text:{juce::String("Oscillators / Sync / FM / Osc1 to Osc2 sync"),juce::String::fromUTF8("Árvíztűrő tükörfúrógép\nMultiline tooltip / 127"),juce::String::repeatedString("Long tooltip information / ",10)}){
  const auto bounds=look.getTooltipBounds(text,{30,30},{0,0,1280,1000});const int w=bounds.getWidth(),h=bounds.getHeight();
  for(float scale:{.5f,.75f,1.f,1.5f,2.f}){
   juce::Image fixed(juce::Image::ARGB,juce::roundToInt(std::ceil(w*scale)),juce::roundToInt(std::ceil(h*scale)),true),native=fixed.createCopy();
   {juce::Graphics g(fixed);g.addTransform(juce::AffineTransform::scale(scale));look.drawTooltip(g,text,w,h);}
   {juce::Graphics g(native);g.addTransform(juce::AffineTransform::scale(scale));look.juce::LookAndFeel_V4::drawTooltip(g,text,w,h);}
   for(auto point:{juce::Point<int>(0,0),juce::Point<int>(fixed.getWidth()-1,0),juce::Point<int>(0,fixed.getHeight()-1),juce::Point<int>(fixed.getWidth()-1,fixed.getHeight()-1)})check(fixed.getPixelAt(point.x,point.y).getAlpha()<(scale>=1.f?1:128),"Tooltip has opaque corner pixels");
   const int margin=juce::roundToInt(std::ceil(8*scale));for(int y=margin;y<fixed.getHeight()-margin;++y)for(int x=margin;x<fixed.getWidth()-margin;++x)check(fixed.getPixelAt(x,y)==native.getPixelAt(x,y),"Tooltip text/layout or central colours changed");
  }
 }
 Processor processor;Editor editor(processor);check(editor.loadSkin(manifest,error),error);auto* tooltip=tooltipWindow(editor);check(tooltip&&!tooltip->isOpaque(),"Real editor tooltip still marked opaque");
 tooltip->displayTip({80,80},"Oscillators / Sync / FM / Osc1 to Osc2 sync");auto tip=tooltip->createComponentSnapshot(tooltip->getLocalBounds(),true,2.f);check(tip.isValid()&&tip.getPixelAt(0,0).getAlpha()==0,"Actual TooltipWindow corner not transparent");
 juce::Image preview(juce::Image::ARGB,tip.getWidth()+32,tip.getHeight()+32,true);{juce::Graphics g(preview);g.fillAll(juce::Colour(0xffced1d5));g.drawImageAt(tip,16,16);}
 auto file=output.getChildFile("tooltip-rounded-corners-2x.png");file.deleteFile();auto stream=file.createOutputStream();check(stream&&juce::PNGImageFormat().writeImageToStream(preview,*stream),"Tooltip preview");tooltip->hideTip();
 std::cout<<"PASS: rounded tooltip transparency at five scales and three text cases; original typography/central pixels preserved; actual editor tooltip is non-opaque\n";return 0;
}

static int releaseChecks(const juce::File& root){
 juce::String error;const auto skins=root.getChildFile("01-Skins"),out=root.getChildFile("06-Verification/Previews");out.createDirectory();
 const auto entries=SkinPackage::discover(skins);check(entries.size()==2,"Release must contain exactly two current skins");
 for(const auto* name:{"Silver-K Blue Standard v1.0","Silver-K Blue Retina 4K v1.0"}){
  const bool retina=juce::String(name).contains("Retina");const auto manifest=skins.getChildFile(name).getChildFile("skin.json");SkinPackage package;check(SkinPackage::load(manifest,package,error),error);check(package.name==name,"Display name mismatch");check(package.id==(retina?"silver-k-blue-retina-4k":"silver-k-blue-standard"),"Stable identity mismatch");
  Processor processor;Editor editor(processor);check(editor.loadSkin(manifest,error),error);editor.setUiScale(1.);
  auto* select=dynamic_cast<juce::ComboBox*>(find(editor,"skinSelect"));check(select&&select->getText()==name,"Selected dropdown text mismatch");
  bool listed=false;for(int i=0;i<select->getNumItems();++i)listed|=select->getItemText(i)==name;check(listed,"Skin name absent from actual dropdown items");
  for(const auto* page:{"pageMain","pagePerf","pagePatches","pageSetup"}){click(editor,page);capture(editor,out.getChildFile((retina?"Retina-":"Standard-")+juce::String(page)+"-2x.png"));}
  auto* tip=tooltipWindow(editor);check(tip&&!tip->isOpaque(),"Tooltip transparency lost");
  if(retina){lcdRevisionTests(package,out);widgetTests(package);}
  tooltipChecks(out.getChildFile(retina?"Retina":"Standard"),manifest);
  std::cout<<"PASS: "<<name<<" package loaded; exact visible dropdown name; all four pages rendered at 2x; rounded tooltip verified\n";
 }
 std::cout<<"PASS: exactly two named release skins; firmware-free final metadata and UI smoke checks\n";return 0;
}

int main(int argc,char** argv){try{
 juce::ScopedJuceInitialiser_GUI gui;if(juce::SystemStats::getEnvironmentVariable("SILVER_K_RELEASE_ONLY",{})=="1"){check(argc>=2,"Release root required");return releaseChecks(juce::File(argv[1]));}if(juce::SystemStats::getEnvironmentVariable("SILVER_K_TOOLTIP_ONLY",{})=="1"){check(argc>=3,"Tooltip output and skin manifest required");return tooltipChecks(juce::File(argv[1]),juce::File(argv[2]));}check(argc>=6,"Usage: test package-root K-ROM KS-ROM KS-DATA-MID WAVE-ROM");const juce::File root(argv[1]);auto previews=root.getChildFile("Previews");previews.createDirectory();juce::String error;
 const auto temp=juce::File(juce::SystemStats::getEnvironmentVariable("SILVER_K_DATA",{}));validation(temp.getChildFile("fixtures"));
 for(const auto* profile:{"Silver-K R2 Navy Native Retina 192","Silver-K R2 Navy Native Retina 256"}){
  const auto manifest=root.getChildFile("Skins").getChildFile(profile).getChildFile("skin.json");SkinPackage pack;check(SkinPackage::load(manifest,pack,error),error);widgetTests(pack);lcdRevisionTests(pack,previews);navyUniformity(pack,root);
  for(int model=0;model<2;++model){
   Processor p;if(model){check(p.setFlashRom(juce::File(argv[4]),error),error);check(p.setWaveRom(juce::File(argv[5]),error),error);}check(p.loadFirmware(juce::File(argv[model+2]),error),error);Editor e(p);check(e.loadSkin(manifest,error),error);e.setUiScale(1.);
   const auto name=juce::String(profile)+(model?"-KS":"-K");click(e,"pageMain");
   if(!model){waveform(e);auto* wave=p.uiParameter("osc1_wave");check(wave!=nullptr,"Wave parameter");for(int n=0;n<4;++n){auto* b=dynamic_cast<juce::TextButton*>(find(e,"osc1_wave:"+juce::String(n)));b->onClick();check(int(wave->convertFrom0to1(wave->getValue()))==n,"Waveform click no longer changes parameter");}}
   check(e.reloadSkin(error),error);const auto before=p.parameters.copyState();
   if(!model){
    click(e,"pagePerf");
    for(int n=1;n<=4;++n)for(const auto* key:{"volume","part_transpose","part_fine"}){
     auto* group=find(e,"performance:part"+juce::String(n)+":"+key);check(group&&std::abs(group->getAlpha()-.35f)<.01f,"Unavailable Performance image group not dimmed");auto* slider=dynamic_cast<juce::Slider*>(find(*group,group->getComponentID()+":slider"));check(slider&&!slider->isEnabled(),"Unavailable K slider enabled");
    }
    auto* volume=find(e,"performance:part1:volume");auto* controls=find(e,"performance:part1:controls");check(volume&&controls,"First part volume");
    const auto bounds=volume->getBounds();auto disabled=controls->createComponentSnapshot(bounds,true,2.f);volume->setAlpha(1.f);auto undimmed=controls->createComponentSnapshot(bounds,true,2.f);volume->setAlpha(.35f);
    auto maxAlpha=[](const juce::Image& im){int peak=0;for(int y=0;y<im.getHeight();++y)for(int x=0;x<im.getWidth();++x)peak=juce::jmax(peak,int(im.getPixelAt(x,y).getAlpha()));return peak;};
    check(maxAlpha(disabled)<maxAlpha(undimmed)*.5f,"First-part fader artwork still looks active");
    std::cout<<"PASS: all 12 unavailable K Performance rotary/fader groups disabled and visibly dimmed; left fader raster alpha reduced\n";
   }

   for(const auto* page:{"pageMain","pagePerf","pagePatches","pageSetup"}){click(e,page);capture(e,previews.getChildFile(name+"-"+page+".png"));}
   check(p.parameters.copyState().isEquivalentTo(before),"Page rendering modified sound parameters");
   click(e,"pageMain");auto* display=dynamic_cast<juce::Label*>(find(e,"programDisplay"));check(display!=nullptr,"Main LCD");const auto savedText=display->getText();display->setText("Preset\nSPRUNG / PART 1: VIBRATINE",juce::dontSendNotification);capture(e,previews.getChildFile(name+"-pageMainLongName.png"));display->setText(savedText,juce::dontSendNotification);
   if(model){
    p.prepareToPlay(48000,256);p.setNonRealtime(true);check(p.setPerformanceMode(true,error),error);
    juce::AudioBuffer<float> audio(juce::jmax(2,p.getTotalNumOutputChannels()),256);juce::MidiBuffer midi;
    for(int n=0;n<150;++n){audio.clear();midi.clear();p.processBlock(audio,midi);}p.refreshControls();
    check(p.performanceMode(),"Performance mode did not activate");check(e.reloadSkin(error),error);click(e,"partTab2");check(p.currentPart()==1,"Part selection");click(e,"pagePerf");
    for(int n=1;n<=4;++n){auto* combo=dynamic_cast<juce::ComboBox*>(find(e,"performance:part"+juce::String(n)+":preset"));check(combo&&combo->isEnabled(),"Active Performance preset field disabled");}
    for(int n=1;n<=4;++n)for(const auto* key:{"volume","part_transpose","part_fine"}){auto* group=find(e,"performance:part"+juce::String(n)+":"+key);check(group&&group->isEnabled()&&group->getAlpha()>.99f,"Active KS Performance control remains dimmed");}
    capture(e,previews.getChildFile(name+"-pagePerfActive.png"));
    std::cout<<"PASS: active four-part Performance, part selection and all four live LCD preset fields\n";
   }

   for(const auto* page:{"pageMain","pagePerf"})for(double scale:{.5,.75,1.,1.5}){click(e,page);e.setUiScale(scale);auto snapshot=e.createComponentSnapshot(e.getLocalBounds(),true,2.f);check(snapshot.isValid(),"GUI scale render");}
   std::cout<<"PASS: "<<name<<" firmware loaded, four pages at 2x raster, parameter state preserved, four GUI sizes\n";
  }
 }
 return 0;
 }catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
