#include "silverSkin.h"
#include "silverImageBounds.h"
#include "silverProcessor.h"
#include "BinaryData.h"
#include "BlueSkinData.h"
#include <set>
#include <cmath>

namespace silverK
{
namespace
{
struct BaseFont final:juce::DynamicObject { explicit BaseFont(juce::Font f):font(std::move(f)){} juce::Font font; };
juce::var object() { return juce::var(new juce::DynamicObject); }
juce::var embedded(const char* name)
{
    int size=0; const auto* data=BinaryData::getNamedResource(name,size);
    return juce::JSON::parse(juce::String::fromUTF8(data,size));
}
juce::var merge(juce::var base, const juce::var& changes)
{
    if (!base.getDynamicObject() || !changes.getDynamicObject()) return changes.clone();
    for (const auto& p:changes.getDynamicObject()->getProperties())
        base.getDynamicObject()->setProperty(p.name,merge(base[p.name],p.value));
    return base;
}
juce::String paletteKey(juce::uint32 value)
{
    switch(value | 0xff000000u)
    {
        case 0xff171b20: return "text";
        case 0xff087fe5: return "accent";
        case 0xfff1f4f7: return "lightText";
        case 0xffced1d5: return "background";
        case 0xff24262a: return "dark";
        case 0xffeef1f5: return "panelTop";
        case 0xffe4eaf0: return "panelBottom";
        case 0xffbdc7cf: return "border";
        case 0xff0752ba: return "display";
        default: return juce::String::toHexString(value | 0xff000000u).paddedLeft('0',8);
    }
}
const std::map<juce::String,int>& juceColourIds()
{
    static const std::map<juce::String,int> ids={
        {"menuBackground",juce::PopupMenu::backgroundColourId},{"menuText",juce::PopupMenu::textColourId},{"menuHighlight",juce::PopupMenu::highlightedBackgroundColourId},{"menuHighlightText",juce::PopupMenu::highlightedTextColourId},
        {"editorBackground",juce::TextEditor::backgroundColourId},{"editorText",juce::TextEditor::textColourId},{"editorOutline",juce::TextEditor::outlineColourId},{"editorFocus",juce::TextEditor::focusedOutlineColourId},{"editorSelection",juce::TextEditor::highlightColourId},{"editorSelectionText",juce::TextEditor::highlightedTextColourId},
        {"tableBackground",juce::ListBox::backgroundColourId},{"tableHeaderBackground",juce::TableHeaderComponent::backgroundColourId},{"tableHeaderText",juce::TableHeaderComponent::textColourId},{"tableHeaderOutline",juce::TableHeaderComponent::outlineColourId},{"tableHeaderHighlight",juce::TableHeaderComponent::highlightColourId},
        {"scrollbarThumb",juce::ScrollBar::thumbColourId},{"scrollbarBackground",juce::ScrollBar::backgroundColourId},{"sliderTrack",juce::Slider::trackColourId},{"sliderThumb",juce::Slider::thumbColourId},{"sliderBackground",juce::Slider::backgroundColourId},
        {"tooltipBackground",juce::TooltipWindow::backgroundColourId},{"tooltipText",juce::TooltipWindow::textColourId},{"tooltipOutline",juce::TooltipWindow::outlineColourId},{"toggleText",juce::ToggleButton::textColourId},{"toggleTick",juce::ToggleButton::tickColourId},{"toggleDisabledTick",juce::ToggleButton::tickDisabledColourId},{"comboText",juce::ComboBox::textColourId}
    };return ids;
}
bool number(const juce::var& v) { return (v.isInt() || v.isInt64() || v.isDouble()) && std::isfinite(double(v)); }
bool rectangle(const juce::var& v)
{
    if (!v.isArray() || v.size()!=4) return false;
    for (int n=0;n<4;++n) if (!number(v[n]) || std::abs(double(v[n]))>16384 || double(v[n])!=int(v[n])) return false;
    return int(v[2])>0 && int(v[3])>0;
}
juce::Rectangle<int> bounds(const juce::var& v) { return {int(v[0]),int(v[1]),int(v[2]),int(v[3])}; }
bool colourString(const juce::var& v)
{
    const auto s=v.toString();return v.isString() && s.startsWithChar('#') && (s.length()==7 || s.length()==9) && s.substring(1).containsOnly("0123456789abcdefABCDEF");
}
juce::Colour parseColour(const juce::var& v)
{
    auto s=v.toString().substring(1);return juce::Colour(juce::uint32(s.getHexValue64()) | (s.length()==6?0xff000000u:0));
}
juce::File localFile(const juce::File& root, const juce::String& path)
{
    if (path.isEmpty() || juce::File::isAbsolutePath(path) || path.containsChar('\\') || path.containsChar(':')) return {};
    const auto parts=juce::StringArray::fromTokens(path,"/","");
    auto f=root;
    for (const auto& part:parts)
    {
        if (part.isEmpty() || part=="." || part=="..") return {};
        f=f.getChildFile(part); if (f.isSymbolicLink()) return {};
    }
    return f.isAChildOf(root)?f:juce::File{};
}
bool readJson(const juce::File& file, juce::var& result, juce::String& error)
{
    if (!file.existsAsFile() || file.getSize()>4*1024*1024) { error="Missing or oversized JSON: "+file.getFileName();return false; }
    const auto status=juce::JSON::parse(file.loadFileAsString(),result);
    if (status.failed() || !result.getDynamicObject()) { error=file.getFileName()+": "+(status.failed()?status.getErrorMessage():"expected an object");return false; }
    return true;
}
bool validControl(const juce::String& id)
{
    if(id.isEmpty())return false;
    juce::StringArray variants{id};
    for (const auto& token:juce::StringArray{"{osc}","{lfo}","{pw}"})
    {
        juce::StringArray next;
        const auto values=token=="{pw}"?juce::StringArray{"pulse_width","lfo2_pwm","env_pwm"}:token=="{osc}"?juce::StringArray{"1","2","3"}:juce::StringArray{"1","2"};
        for (const auto& value:variants) { if (!value.contains(token)) next.add(value);else for(const auto& replacement:values)next.add(value.replace(token,replacement)); }
        variants=next;
    }
    static const auto known=[] { std::set<juce::String> ids;for(const auto& c:ksPlugin::controls)ids.insert(semanticId(c.id));for(const auto& c:kControls())ids.insert(semanticId(c.id));return ids; }();
    for(const auto& variant:variants)if(!known.count(semanticId(variant)))return false;
    return true;
}
bool validateLayout(const juce::var& l, juce::String& error)
{
    auto fail=[&](const juce::String& s){error="Layout: "+s;return false;};
    if(!number(l["width"]) || !number(l["height"]) || int(l["width"])<320 || int(l["width"])>4096 || int(l["height"])<320 || int(l["height"])>4096)return fail("width and height must be between 320 and 4096");
    if(l["layout"].toString()!="studio" && l["layout"].toString()!="legacy")return fail("layout must be studio or legacy");
    for(const auto& key:juce::StringArray{"header","title","lcd","fmButton","effectMenuArea","effectSelect"})if(!rectangle(l[juce::Identifier(key)]))return fail("invalid rectangle: "+key);
    const auto canvas=l["performanceCanvas"];
    if(!canvas.isArray() || canvas.size()!=2 || !number(canvas[0]) || !number(canvas[1]) || int(canvas[0])<320 || int(canvas[0])>4096 || int(canvas[1])<320 || int(canvas[1])>4096)return fail("invalid performanceCanvas size");
    const auto baseline=embedded("silverk_json");
    for(const auto& key:juce::StringArray{"actions","advanced","setup","selectors","studioHeader","studioRegions"})
    {
        const auto node=l[juce::Identifier(key)], ref=baseline[juce::Identifier(key)];
        if(!node.getDynamicObject())return fail("expected object: "+key);
        for(const auto& p:ref.getDynamicObject()->getProperties())
        {
            const auto v=node[p.name];
            if(p.value.isArray() && p.value.size()==4 && number(p.value[0])) { if(!rectangle(v))return fail("invalid rectangle: "+key+"."+p.name.toString()); }
            else if(p.value.isArray())
            {
                if(!v.isArray() || v.size()!=p.value.size())return fail("invalid array: "+key+"."+p.name.toString());
                for(const auto& r:*v.getArray())if(!rectangle(r))return fail("invalid rectangle: "+key+"."+p.name.toString());
            }
            else if(!number(v) || int(v)<1 || int(v)>4096)return fail("invalid size: "+key+"."+p.name.toString());
        }
    }
    for(const auto& key:juce::StringArray{"knobDiameter","cutoffDiameter","classicDiameter"})if(!number(l[juce::Identifier(key)]) || int(l[juce::Identifier(key)])<4 || int(l[juce::Identifier(key)])>512)return fail("invalid knob size: "+key);
    const juce::StringArray kinds{"menu","knob","mixer","compact","row","extraRow","fader","led","lamps","waveButtons","panelKnob","panelMenu","classic","miniKnob","inline","rhythm"};
    auto controls=[&](const juce::var& list){
        if(!list.isArray() || list.size()>512)return false;
        std::set<juce::String> seen;
        for(const auto& c:*list.getArray())if(!c.getDynamicObject() || !validControl(c["id"].toString()) || !seen.insert(c["id"].toString()).second || !kinds.contains(c["kind"].toString()) || !rectangle(c["rect"]))return false;
        return true;
    };
    if(!controls(l["controls"]) || !controls(l["studioModels"]["K"]) || !controls(l["studioModels"]["KS"]))return fail("invalid controls, duplicate IDs or unknown parameter/kind");
    if(!l["sections"].isArray() || l["sections"].size()>128)return fail("invalid sections");
    for(const auto& s:*l["sections"].getArray()){
        if(!s.isArray() || s.size()!=5 || !s[4].isString())return fail("invalid section");
        juce::Array<juce::var> r;for(int n=0;n<4;++n)r.add(s[n]);if(!rectangle(juce::var(r)))return fail("invalid section rectangle");
    }
    for(const auto& key:juce::StringArray{"pwmSelectors","lfoSelectors","effectSlots","effectMenus"}){
        const auto list=l[juce::Identifier(key)];if(!list.isArray() || list.size()!=baseline[juce::Identifier(key)].size())return fail("invalid array: "+key);
        for(const auto& r:*list.getArray())if(!rectangle(r))return fail("invalid rectangle: "+key);
    }
    if(!l["effects"].isArray() || l["effects"].size()<1 || l["effects"].size()>32)return fail("expected 1 to 32 effects");
    for(const auto& fx:*l["effects"].getArray()){
        if(!fx.getDynamicObject() || fx["name"].toString().isEmpty() || !fx["labels"].isArray())return fail("invalid effect");
        for(const auto& key:juce::StringArray{"controls","menus"}){
            const auto list=fx[juce::Identifier(key)];if(!list.isArray() || list.size()>(key=="controls"?6:16))return fail("invalid effect "+key);
            for(const auto& id:*list.getArray())if(!validControl(id.toString()))return fail("unknown effect parameter: "+id.toString());
        }
        if(fx["labels"].size()!=fx["controls"].size())return fail("effect labels do not match controls");
    }
    if(l.hasProperty("components")){
        if(!l["components"].getDynamicObject() || l["components"].getDynamicObject()->getProperties().size()>4096)return fail("invalid components");
        for(const auto& p:l["components"].getDynamicObject()->getProperties())if(!p.value.getDynamicObject() || (p.value.hasProperty("bounds")&&!rectangle(p.value["bounds"])))return fail("invalid component: "+p.name.toString());
    }
    if(l.hasProperty("decorations")){
        if(!l["decorations"].isArray() || l["decorations"].size()>512)return fail("invalid decorations");
        std::set<juce::String> ids;
        for(const auto& d:*l["decorations"].getArray())if(!d.getDynamicObject() || !rectangle(d["rect"]) || !juce::StringArray{"image","text","panel"}.contains(d["kind"].toString()) || d["id"].toString().isEmpty() || !ids.insert(d["id"].toString()).second || (d.hasProperty("colour")&&!colourString(d["colour"])) || (d.hasProperty("fontSize")&&(!number(d["fontSize"]) || double(d["fontSize"])<4 || double(d["fontSize"])>256)))return fail("invalid decoration");
    }
    return true;
}
const SkinLookAndFeel* look(const juce::Component& c) { return dynamic_cast<const SkinLookAndFeel*>(&c.getLookAndFeel()); }
juce::String componentPath(const juce::Component& c)
{
    const auto id=c.getComponentID();if(id=="surface")return {};
    const auto* parent=c.getParentComponent();const auto prefix=parent?componentPath(*parent):juce::String{};
    return id.isEmpty()?prefix:prefix.isEmpty()?id:prefix+"/"+id;
}
juce::var componentOptions(const juce::var& map,const juce::Component& c)
{
    if(c.getComponentID().isEmpty())return {};
    const auto byId=map[juce::Identifier(c.getComponentID())];const auto path=componentPath(c);const auto byPath=path.isEmpty()?juce::var{}:map[juce::Identifier(path)];
    return byPath.getDynamicObject()?merge(byId.clone(),byPath):byId;
}
}

SkinPackage SkinPackage::builtin()
{
    SkinPackage s;s.id="builtin";s.name="Silver-K";s.layout=embedded("silverk_json");s.theme=embedded("silverktheme_json");
    const std::pair<const char*,const char*> assets[]={{"knobBody","silverkknobbody_png"},{"knobGrip","silverkknobgrip_png"},{"knobBase","silverkstandknobbase_png"},{"buttonOff","silverkbuttonoff_png"},{"buttonOn","silverkbuttonon_png"}};
    for(const auto& a:assets){int size=0;const auto* data=BinaryData::getNamedResource(a.second,size);s.images[a.first]=juce::ImageFileFormat::loadFrom(data,size);}
    return s;
}
static bool loadPackage(const juce::File& file,SkinPackage& result,juce::String& error,juce::ZipFile* archive=nullptr)
{
    auto candidate=SkinPackage::builtin();juce::var manifest;
    const auto readBytes=[&](const juce::String& relative,juce::int64 limit,juce::MemoryBlock& bytes)->bool{
        if(archive){
            if(juce::File::isAbsolutePath(relative)||relative.contains("..")||relative.containsChar('\\')){error="Invalid embedded resource path";return false;}
            const int index=archive->getIndexOfFileName(relative);
            if(index<0 || archive->getEntry(index)->uncompressedSize>limit){error="Missing or oversized embedded resource: "+relative;return false;}
            std::unique_ptr<juce::InputStream> stream(archive->createStreamForEntry(index));
            if(!stream || stream->readIntoMemoryBlock(bytes)!=archive->getEntry(index)->uncompressedSize){error="Incomplete embedded resource";return false;}
            return true;
        }
        const auto root=file.isDirectory()?file:file.getParentDirectory();const auto resource=localFile(root,relative);
        if(!resource.existsAsFile()||resource.getSize()>limit||!resource.loadFileAsData(bytes)){error="Missing or oversized resource: "+relative;return false;}
        return true;
    };
    const auto readPart=[&](const juce::String& relative,juce::var& value)->bool{
        juce::MemoryBlock bytes;if(!readBytes(relative,4*1024*1024,bytes))return false;
        const auto status=juce::JSON::parse(juce::String::fromUTF8(static_cast<const char*>(bytes.getData()),int(bytes.getSize())),value);
        if(status.failed()||!value.isObject()){error="Invalid skin JSON: "+relative;return false;}return true;
    };
    const auto entry=file.isDirectory()?file.getChildFile("skin.json"):file;
    if(archive?!readPart("skin.json",manifest):!readJson(entry,manifest,error))return false;
    candidate.manifest=archive?juce::File{}:entry;
    if(manifest.hasProperty("schemaVersion")){
        if(!manifest["schemaVersion"].isInt() || int(manifest["schemaVersion"])!=1 || manifest["engine"].toString()!="silver-k-juce"){error="Unsupported skin schemaVersion or engine";return false;}
        candidate.id=manifest["id"].toString();candidate.name=manifest["name"].toString();
        if(candidate.id.isEmpty() || candidate.id.length()>128 || !candidate.id.containsOnly("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._-") || candidate.name.isEmpty()){error="Skin needs a name and an ID using letters, digits, dots, underscores or hyphens";return false;}
        juce::var layout,theme;
        if(!readPart(manifest["layout"].toString(),layout)||!readPart(manifest["theme"].toString(),theme))return false;
        candidate.layout=merge(candidate.layout,layout);candidate.theme=merge(candidate.theme,theme);
        const auto images=candidate.theme["images"];
        if(!images.getDynamicObject()){error="Theme images must be an object";return false;}
        uint64_t totalImagePixels=0;
        for(const auto& p:images.getDynamicObject()->getProperties()){
            juce::MemoryBlock bytes;if(!readBytes(p.value.toString(),32*1024*1024,bytes))return false;
            uint64_t pixels=0;
            if(!boundedImage(bytes.getData(),bytes.getSize(),pixels) || pixels>128ull*1024*1024-totalImagePixels){error="Image dimensions or total skin image memory exceed limits: "+p.value.toString();return false;}
            totalImagePixels+=pixels;
            auto image=juce::ImageFileFormat::loadFrom(bytes.getData(),bytes.getSize());
            // A 128-frame Retina strip is bounded to 256px per square frame.
            const int shortEdge=juce::jmin(image.getWidth(),image.getHeight());
            const int longEdge=juce::jmax(image.getWidth(),image.getHeight());
            const bool retinaStrip=shortEdge>0 && shortEdge<=256 && longEdge==shortEdge*128;
            if(!image.isValid() || ((image.getWidth()>8192 || image.getHeight()>8192) && !retinaStrip)){error="Invalid image: "+p.value.toString();return false;}
            candidate.images[p.name.toString()]=image;
        }
        const auto font=candidate.theme["font"]["file"].toString();
        if(font.isNotEmpty()){
            juce::MemoryBlock bytes;
            if(!readBytes(font,16*1024*1024,bytes) || !(candidate.typeface=juce::Typeface::createSystemTypefaceFor(bytes.getData(),bytes.getSize()))){error="Invalid font: "+font;return false;}
        }
    }else{
        // Existing standalone Silver-K layout JSONs remain readable.
        if(!manifest["controls"].isArray() || !manifest["effects"].isArray()){error="Expected a skin package manifest or a Silver-K layout JSON";return false;}
        candidate.id="legacy";candidate.name=manifest["name"].toString();candidate.layout=merge(candidate.layout,manifest);if(manifest["layout"].toString()!="studio")candidate.layout.getDynamicObject()->setProperty("layout","legacy");
    }
    if(!validateLayout(candidate.layout,error))return false;
    const auto palette=candidate.theme["palette"];
    if(!palette.getDynamicObject()){error="Theme palette must be an object";return false;}
    for(const auto& p:palette.getDynamicObject()->getProperties())if(!colourString(p.value)){error="Invalid palette colour: "+p.name.toString();return false;}
    if(candidate.theme.hasProperty("renderStyle") && !juce::StringArray{"classic","flat"}.contains(candidate.theme["renderStyle"].toString())){error="Theme renderStyle must be classic or flat";return false;}
    const auto font=candidate.theme["font"];
    if(!font.getDynamicObject() || !number(font["scale"]) || double(font["scale"])<.5 || double(font["scale"])>2){error="Font scale must be between 0.5 and 2";return false;}
    const auto juceColours=candidate.theme["juceColours"];
    if(!juceColours.getDynamicObject()){error="Theme juceColours must be an object";return false;}
    for(const auto& p:juceColours.getDynamicObject()->getProperties())if(!juceColourIds().count(p.name.toString()) || !colourString(p.value)){error="Invalid JUCE colour: "+p.name.toString();return false;}
    const auto validWidget=[&](const juce::var& owner)->bool{
        for(const auto* section:{"lcd","comboBoxLcd"})if(owner.hasProperty(section)){
            const auto cfg=owner[juce::Identifier(section)];
            if(!cfg.isObject()){error="Invalid widget config: "+juce::String(section);return false;}
            for(const auto* key:{"enabled","dotMatrixText","wrap","fitText"})if(cfg.hasProperty(key)&&!cfg[juce::Identifier(key)].isBool()){error="Widget flag must be boolean: "+juce::String(key);return false;}
            const auto range=[&](const char* key,double low,double high,bool integer=false){const auto v=cfg[juce::Identifier(key)];return !cfg.hasProperty(key)||(number(v)&&double(v)>=low&&double(v)<=high&&(!integer||double(v)==int(v)));};
            if(!range("columns",8,80,true)||!range("rows",1,4,true)||!range("dotSize",.5,4)||!range("inactiveAlpha",0,.3)||!range("gridAlpha",0,.3)||!range("gridPitch",1,8)){error="Widget numeric value out of range";return false;}
            for(const auto* key:{"textColour","topColour","bottomColour","gridColour","borderColour"})if(cfg.hasProperty(key)&&!colourString(cfg[juce::Identifier(key)])){error="Invalid widget colour";return false;}
        }
        return true;
    };
    if(!validWidget(candidate.theme))return false;
    const auto styles=candidate.theme["components"];
    if(!styles.getDynamicObject()){error="Theme components must be an object";return false;}
    for(const auto& p:styles.getDynamicObject()->getProperties()){
        const auto style=p.value;
        if(!style.getDynamicObject()){error="Invalid component style: "+p.name.toString();return false;}
        if(!validWidget(style))return false;
        if(style.hasProperty("waveformHoverOnly") && !style["waveformHoverOnly"].isBool()){error="waveformHoverOnly must be boolean";return false;}
        if(style.hasProperty("faderThumbScale") && (!number(style["faderThumbScale"]) || double(style["faderThumbScale"])<1. || double(style["faderThumbScale"])>8.)){error="Invalid component faderThumbScale";return false;}
        if(style.hasProperty("fontSize") && (!number(style["fontSize"]) || double(style["fontSize"])<4 || double(style["fontSize"])>256)){error="Invalid component fontSize";return false;}
        if(style.hasProperty("palette")){
            if(!style["palette"].getDynamicObject()){error="Invalid component palette";return false;}
            for(const auto& colour:style["palette"].getDynamicObject()->getProperties())if(!colourString(colour.value)){error="Invalid component colour";return false;}
        }
        for(const auto& key:juce::StringArray{"backgroundImage","knobStrip","buttonOff","buttonOn","faderTrack","faderThumb"})if(style.hasProperty(juce::Identifier(key))&&!candidate.images.count(style[juce::Identifier(key)].toString())){error="Unknown image in component style: "+key;return false;}
    }
    if(candidate.theme.hasProperty("knobStrip")){
        const auto strip=candidate.theme["knobStrip"];const int frames=int(strip["frames"]);const auto found=candidate.images.find(strip["image"].toString());
        if(!strip.getDynamicObject() || frames<2 || frames>1024 || found==candidate.images.end() || !juce::StringArray{"vertical","horizontal"}.contains(strip["orientation"].toString()) || (strip["orientation"].toString()=="horizontal"?found->second.getWidth():found->second.getHeight())%frames!=0){error="Invalid knobStrip image, frame count or orientation";return false;}
    }
    for(const auto& p:styles.getDynamicObject()->getProperties())if(p.value.hasProperty("knobStrip")){
        const auto config=candidate.theme["knobStrip"];const int frames=int(config["frames"]);const auto& bitmap=candidate.images.at(p.value["knobStrip"].toString());
        if(frames<2 || frames>1024 || (config["orientation"].toString()=="horizontal"?bitmap.getWidth():bitmap.getHeight())%frames!=0){error="Component knobStrip does not match global frame geometry";return false;}
    }
    if(auto* array=candidate.layout["decorations"].getArray())for(const auto& d:*array){
        if(d["kind"].toString()=="image"&&!candidate.images.count(d["image"].toString())){error="Unknown decoration image: "+d["image"].toString();return false;}
        juce::StringArray parents{"","surface","mainPanel","patchBrowser","performancePanel","performanceCanvas","localExtra","advancedContent"};
        for(int n=1;n<=4;++n){parents.add("performance:part"+juce::String(n)+":strip");parents.add("performance:part"+juce::String(n)+":controls");}
        if(!parents.contains(d["parent"].toString())){error="Unknown decoration parent: "+d["parent"].toString();return false;}
    }
    result=std::move(candidate);error.clear();return true;
}
bool SkinPackage::load(const juce::File& file,SkinPackage& result,juce::String& error)
{ return loadPackage(file,result,error); }
juce::StringArray SkinPackage::choices()
{ return {"Auto","Silver-K","KS Grey","Classic","Silver-K Blue Standard v1.0","Silver-K Blue Retina 4K v1.0"}; }
bool SkinPackage::loadEmbedded(const juce::String& choice,SkinPackage& result,juce::String& error)
{
    const auto index=choices().indexOf(choice);
    if(index<4){error="Unknown embedded Blue skin";return false;}
    juce::MemoryOutputStream data(64*1024*1024);
    const juce::String prefix=index==4?"bluestandard":"blueretina";
    for(int chunk=0;;++chunk){
        int size=0;const auto* bytes=BlueSkinData::getNamedResource((prefix+juce::String(chunk)+"_bin").toRawUTF8(),size);
        if(!bytes)break;
        if(size<=0 || size>4*1024*1024 || data.getDataSize()+size>96*1024*1024){error="Invalid embedded skin chunk";return false;}
        data.write(bytes,size);
    }
    if(data.getDataSize()==0){error="Embedded Blue skin missing";return false;}
    juce::MemoryInputStream stream(data.getData(),data.getDataSize(),false);juce::ZipFile archive(stream);
    return loadPackage(juce::File::getCurrentWorkingDirectory().getChildFile("skin.json"),result,error,&archive);
}
std::vector<std::pair<juce::String,juce::File>> SkinPackage::discover(const juce::File& folder)
{
    std::vector<std::pair<juce::String,juce::File>> result;
    auto entries=folder.findChildFiles(juce::File::findDirectories,false);entries.sort();
    for(const auto& dir:entries){const auto file=dir.getChildFile("skin.json");juce::var manifest;juce::String ignored;if(file.existsAsFile()&&readJson(file,manifest,ignored))result.emplace_back(manifest.getProperty("name",dir.getFileName()).toString(),file);}
    return result;
}
bool SkinPackage::exportTemplate(const juce::File& folder,juce::File& result,juce::String& error)
{
    if(const auto status=folder.createDirectory();status.failed()){error=status.getErrorMessage();return false;}
    auto target=folder.getNonexistentChildFile("Silver-K Template",{},false);
    if(const auto status=target.getChildFile("assets").createDirectory();status.failed()){error=status.getErrorMessage();return false;}
    auto manifest=object();auto* m=manifest.getDynamicObject();m->setProperty("schemaVersion",1);m->setProperty("engine","silver-k-juce");m->setProperty("id","silver-k-"+juce::Uuid().toString());m->setProperty("name",target.getFileName());m->setProperty("author","");m->setProperty("layout","layout.json");m->setProperty("theme","theme.json");
    auto skin=builtin();auto theme=skin.theme.clone();auto images=object();
    const std::pair<const char*,const char*> assets[]={{"knobBody","silverkknobbody_png"},{"knobGrip","silverkknobgrip_png"},{"knobBase","silverkstandknobbase_png"},{"buttonOff","silverkbuttonoff_png"},{"buttonOn","silverkbuttonon_png"}};
    bool ok=true;
    for(const auto& a:assets){int size=0;const auto* data=BinaryData::getNamedResource(a.second,size);const auto name=juce::String("assets/")+a.first+".png";ok=ok&&data&&target.getChildFile(name).replaceWithData(data,size);images.getDynamicObject()->setProperty(a.first,name);}
    theme.getDynamicObject()->setProperty("images",images);
    ok=ok&&target.getChildFile("layout.json").replaceWithText(juce::JSON::toString(skin.layout))&&target.getChildFile("theme.json").replaceWithText(juce::JSON::toString(theme));
    // Publish the manifest last so scans never list an incomplete export.
    ok=ok&&target.getChildFile("skin.json").replaceWithText(juce::JSON::toString(manifest));
    if(!ok){target.deleteRecursively();error="Cannot export skin template";return false;}
    result=target.getChildFile("skin.json");return true;
}

SkinLookAndFeel::SkinLookAndFeel(const SkinPackage& s):package(s)
{
    const std::pair<int,juce::uint32> colours[]={{juce::PopupMenu::backgroundColourId,0xff202a34},{juce::PopupMenu::textColourId,0xfff1f4f7},{juce::PopupMenu::highlightedBackgroundColourId,0xff087fe5},{juce::ComboBox::textColourId,0xfff1f4f7},{juce::ToggleButton::textColourId,0xff171b20},{juce::ToggleButton::tickColourId,0xff087fe5},{juce::TextEditor::textColourId,0xff171b20},{juce::TextEditor::backgroundColourId,0xfff1f4f7},{juce::TextEditor::outlineColourId,0xffbdc7cf},{juce::TextEditor::focusedOutlineColourId,0xff087fe5},{juce::ListBox::backgroundColourId,0xffe5e7ea},{juce::TableHeaderComponent::backgroundColourId,0xffced1d5},{juce::TableHeaderComponent::textColourId,0xff171b20},{juce::TableHeaderComponent::outlineColourId,0xffbdc7cf},{juce::TableHeaderComponent::highlightColourId,0x33087fe5},{juce::ScrollBar::thumbColourId,0xffbdc7cf},{juce::Slider::trackColourId,0xff087fe5},{juce::Slider::thumbColourId,0xfff1f4f7},{juce::Slider::backgroundColourId,0xff24262a},{juce::TooltipWindow::backgroundColourId,0xff202a34},{juce::TooltipWindow::textColourId,0xfff1f4f7},{juce::TooltipWindow::outlineColourId,0xffbdc7cf}};
    for(const auto& c:colours)setColour(c.first,colour(c.second));
    setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);setColour(juce::Slider::textBoxBackgroundColourId,juce::Colours::transparentBlack);setColour(juce::ToggleButton::tickDisabledColourId,colour(0xff171b20).withAlpha(.4f));
    for(const auto& p:package.theme["juceColours"].getDynamicObject()->getProperties()){
        const auto found=juceColourIds().find(p.name.toString());if(found!=juceColourIds().end())setColour(found->second,parseColour(p.value));
    }
}
juce::Colour SkinLookAndFeel::colour(juce::uint32 original)const
{
    const auto c=juce::Colour(original);const auto v=package.theme["palette"][juce::Identifier(paletteKey(original))];
    return colourString(v)?parseColour(v).withMultipliedAlpha(c.getFloatAlpha()):c;
}
juce::Font SkinLookAndFeel::font(float height,int style)const
{
    const auto settings=package.theme["font"];height*=float(settings.getProperty("scale",1.));
    if(package.typeface)return juce::Font(package.typeface).withHeight(height).withStyle(style);
    const auto family=settings["family"].toString();return family.isEmpty()?juce::Font(height,style):juce::Font(family,height,style);
}
const juce::Image& SkinLookAndFeel::image(const juce::String& id)const{const auto it=package.images.find(id);static const juce::Image empty;return it==package.images.end()?empty:it->second;}
juce::Typeface::Ptr SkinLookAndFeel::getTypefaceForFont(const juce::Font& f){return package.typeface?package.typeface:LookAndFeel_V4::getTypefaceForFont(f);}
juce::Font SkinLookAndFeel::getPopupMenuFont(){const auto base=LookAndFeel_V4::getPopupMenuFont();return font(base.getHeight(),base.getStyleFlags());}
juce::Font SkinLookAndFeel::getTextButtonFont(juce::TextButton&,int){return font(13.f,juce::Font::bold);}
namespace {
const char* hardwareLcdGlyph(juce::juce_wchar c)
{
    switch(c) {
        case 'A': return "01110100011000111111100011000110001";
        case 'B': return "11110100011000111110100011000111110";
        case 'C': return "01111100001000010000100001000001111";
        case 'D': return "11110100011000110001100011000111110";
        case 'E': return "11111100001000011110100001000011111";
        case 'F': return "11111100001000011110100001000010000";
        case 'G': return "01111100001000010111100011000101111";
        case 'H': return "10001100011000111111100011000110001";
        case 'I': return "11111001000010000100001000010011111";
        case 'J': return "00111000100001000010000101001001100";
        case 'K': return "10001100101010011000101001001010001";
        case 'L': return "10000100001000010000100001000011111";
        case 'M': return "10001110111010110101100011000110001";
        case 'N': return "10001110011010110011100011000110001";
        case 'O': return "01110100011000110001100011000101110";
        case 'P': return "11110100011000111110100001000010000";
        case 'Q': return "01110100011000110001101011001001101";
        case 'R': return "11110100011000111110101001001010001";
        case 'S': return "01111100001000001110000010000111110";
        case 'T': return "11111001000010000100001000010000100";
        case 'U': return "10001100011000110001100011000101110";
        case 'V': return "10001100011000110001100010101000100";
        case 'W': return "10001100011000110101101011101110001";
        case 'X': return "10001100010101000100010101000110001";
        case 'Y': return "10001100010101000100001000010000100";
        case 'Z': return "11111000010001000100010001000011111";
        case '0': return "01110100011001110101110011000101110";
        case '1': return "00100011000010000100001000010001110";
        case '2': return "01110100010000100010001000100011111";
        case '3': return "11110000010000101110000010000111110";
        case '4': return "00010001100101010010111110001000010";
        case '5': return "11111100001000011110000010000111110";
        case '6': return "01110100001000011110100011000101110";
        case '7': return "11111000010001000100010000100001000";
        case '8': return "01110100011000101110100011000101110";
        case '9': return "01110100011000101111000010000101110";
        case ' ': return "00000000000000000000000000000000000";
        case '_': return "00000000000000000000000000000011111";
        case '-': return "00000000000000011111000000000000000";
        case '+': return "00000001000010011111001000010000000";
        case '.': return "00000000000000000000000000011000110";
        case ':': return "00000001100011000000001100011000000";
        case '/': return "00001000100001000100010000100010000";
        case '?': return "01110100010000100010001000000000100";
        case '%': return "11001110100010000100010001011000110";
        case '(': return "00010001000100001000010000010000010";
        case ')': return "01000001000001000010000100010001000";
        case '=': return "00000000001111100000111110000000000";
        case '<': return "00010001000100010000010000010000010";
        case '>': return "01000001000001000001000100010001000";
        case ',': return "00000000000000000000001100010001000";
        case '!': return "00100001000010000100001000000000100";
        case '#': return "01010111110101001010111110101000000";
        case '*': return "00000101010111011111011101010100000";
        case ';': return "00000001100011000000001100010001000";
        case '\'': return "00100001000001000000000000000000000";
        case '"': return "01010010100101000000000000000000000";
        case '[': return "01110010000100001000010000100001110";
        case ']': return "01110000100001000010000100001001110";
        case '&': return "01100100101010001000101011001001101";
        case '@': return "01110100011011110101101111000001111";
        case '\\': return "10000010000100000100000100001000001";
        case '^': return "00100010101000100000000000000000000";
        case '|': return "00100001000010000100001000010000100";
        case '$': return "00100011111010001110001011111000100";
        case '~': return "00000000000110110010000000000000000";
        case '`': return "01000001000001000000000000000000000";
        case '{': return "00010001000010001000001000010000010";
        case '}': return "01000001000010000010001000010001000";
        default: return "01110100010000100010001000000000100";
    }
}
}

juce::var SkinLookAndFeel::widgetStyle(const juce::Component& c,const char* section) const
{
    const auto local=c.getProperties()["silverSkinStyle"][juce::Identifier(section)];
    return local.isObject()?merge(package.theme[juce::Identifier(section)].clone(),local):package.theme[juce::Identifier(section)];
}
namespace {
juce::String lcdAlphabet(const juce::String& text)
{
    juce::String out;
    for(auto c:text.toUpperCase()){
        if(c==0x00c1 || c==0x00e1 || c==0x00c0 || c==0x00e0 || c==0x00c2 || c==0x00e2 || c==0x00c4 || c==0x00e4 || c==0x00c3 || c==0x00e3)c='A';
        else if(c==0x00c9 || c==0x00e9 || c==0x00c8 || c==0x00e8 || c==0x00ca || c==0x00ea || c==0x00cb || c==0x00eb)c='E';
        else if(c==0x00cd || c==0x00ed || c==0x00cc || c==0x00ec || c==0x00ce || c==0x00ee || c==0x00cf || c==0x00ef)c='I';
        else if(c==0x00d3 || c==0x00f3 || c==0x00d2 || c==0x00f2 || c==0x00d4 || c==0x00f4 || c==0x00d6 || c==0x00f6 || c==0x0150 || c==0x0151)c='O';
        else if(c==0x00da || c==0x00fa || c==0x00d9 || c==0x00f9 || c==0x00db || c==0x00fb || c==0x00dc || c==0x00fc || c==0x0170 || c==0x0171)c='U';
        else if(c==0x00c7 || c==0x00e7)c='C';else if(c==0x00d1 || c==0x00f1)c='N';else if(c==0x00dd || c==0x00fd)c='Y';
        else if(c==0x2013 || c==0x2014 || c==0x2212)c='-';
        else if(c==0x2026){out+="...";continue;}
        else if(c==0x00df){out+="SS";continue;}
        else if(c=='\t')c=' ';
        if(c<32 || c>126)return {}; // Let native JUCE render scripts outside the hardware alphabet.
        out+=juce::String::charToString(c);
    }
    return out;
}
}
juce::StringArray SkinLookAndFeel::wrapLcdLines(const juce::String& text,int columns)
{
    columns=juce::jlimit(1,80,columns);
    juce::StringArray result,paragraphs;paragraphs.addLines(text.replace("\r\n","\n").replace("\r","\n"));
    for(auto line:paragraphs){
        while(line.length()>columns){
            const int space=line.substring(0,columns+1).lastIndexOfChar(' ');
            const int split=space>0?space:columns;
            result.add(line.substring(0,split).trimEnd());line=line.substring(split).trimStart();
        }
        result.add(line);
    }
    return result;
}
bool SkinLookAndFeel::drawLcdText(juce::Graphics& g,const juce::Component& c,const juce::String& text,
                                 juce::Rectangle<float> area,int rows,const char* section) const
{
    const auto config=widgetStyle(c,section);
    if(!bool(config["enabled"]) || area.isEmpty())return false;
    rows=juce::jlimit(1,4,rows);
    const float requestedPitch=float(config.getProperty("dotSize",1.8));
    const bool combo=juce::String(section)=="comboBoxLcd";
    const int defaultColumns=combo?juce::jlimit(1,80,int(area.getWidth()/(6.f*requestedPitch))):24;
    const int columns=juce::jlimit(1,80,int(config.getProperty("columns",defaultColumns)));
    const bool wrap=rows>1 && bool(config.getProperty("wrap",false));
    juce::StringArray lines;
    if(wrap)lines=wrapLcdLines(text,columns);else lines.addLines(text.replace("\r\n","\n").replace("\r","\n"));
    juce::StringArray display;
    for(const auto& line:lines){const auto normalized=lcdAlphabet(line);if(line.isNotEmpty() && normalized.isEmpty())return false;display.add(normalized);}
    // Wrapping uses fixed-size cells. Legacy skins may still request single-line fitting.
    int count=columns;
    if(!wrap && bool(config.getProperty("fitText",!combo))){
        for(const auto& line:display)count=juce::jmax(count,line.length());
        count=juce::jmin(count,juce::jmax(columns,int(area.getWidth()/5.f)));
        count=juce::jmin(80,count);
    }
    const float pitch=juce::jmin(float(config.getProperty("dotSize",1.8)),juce::jmin(area.getWidth()/(count*6.f),area.getHeight()/(rows*10.f)));
    if(pitch<=0.f)return true;
    const float y0=area.getCentreY()-rows*10.f*pitch*.5f+pitch;
    const auto foreground=config.hasProperty("textColour")?parseColour(config["textColour"]):juce::Colour(0xffe9f5ff);
    const float disabled=c.isEnabled()?1.f:.45f;
    const float inactive=juce::jlimit(0.f,.3f,float(config.getProperty("inactiveAlpha",.055)));
    const juce::Graphics::ScopedSaveState save(g);g.reduceClipRegion(area.toNearestInt());
    for(int row=0;row<rows;++row){
        auto line=row<display.size()?display[row]:juce::String{};
        if(row==rows-1 && display.size()>rows)line+="...";
        if(line.length()>count)line=line.substring(0,juce::jmax(0,count-3))+"...";
        for(int col=0;col<count;++col){
            const char* bits=hardwareLcdGlyph(col<line.length()?line[col]:' ');
            for(int yy=0;yy<7;++yy)for(int xx=0;xx<5;++xx){
                const bool on=bits[yy*5+xx]=='1';
                g.setColour(foreground.withMultipliedAlpha((on?1.f:inactive)*disabled));
                g.fillRect(area.getX()+(col*6+xx)*pitch,y0+(row*10+yy)*pitch,pitch*.78f,pitch*.78f);
            }
        }
    }
    return true;
}
void SkinLookAndFeel::drawLabel(juce::Graphics& g,juce::Label& label)
{
    if(!label.isBeingEdited()){
        if(label.getComponentID()=="programDisplay"){
            g.fillAll(label.findColour(juce::Label::backgroundColourId));
            const int rows=int(widgetStyle(label,"lcd").getProperty("rows",2));
            if(drawLcdText(g,label,label.getText(),label.getBorderSize().subtractedFrom(label.getLocalBounds()).toFloat(),rows))return;
        }
        if(auto* combo=dynamic_cast<juce::ComboBox*>(label.getParentComponent())){
            if(bool(combo->getProperties()["performancePreset"])){
                if(drawLcdText(g,*combo,label.getText(),label.getBorderSize().subtractedFrom(label.getLocalBounds()).toFloat(),1))return;
            }else if(bool(widgetStyle(*combo,"comboBoxLcd")["dotMatrixText"])){
                if(drawLcdText(g,*combo,label.getText(),label.getBorderSize().subtractedFrom(label.getLocalBounds()).toFloat(),1,"comboBoxLcd"))return;
            }
        }
    }
    LookAndFeel_V4::drawLabel(g,label);
}
namespace {
const juce::ComboBox* menuCombo(const juce::PopupMenu::Options& options)
{
    if(auto* c=dynamic_cast<juce::ComboBox*>(options.getTargetComponent()))return c;
    return dynamic_cast<juce::ComboBox*>(options.getTopLevelTargetComponent());
}
}
void SkinLookAndFeel::drawPopupMenuItemWithOptions(juce::Graphics& g,const juce::Rectangle<int>& area,
        bool highlighted,const juce::PopupMenu::Item& item,const juce::PopupMenu::Options& options)
{
    const auto* combo=menuCombo(options);
    const auto config=combo?widgetStyle(*combo,"comboBoxLcd"):juce::var{};
    if(!combo || !bool(config["enabled"]) || !bool(config["dotMatrixText"]) || item.isSeparator || item.isSectionHeader || (item.text.isNotEmpty() && lcdAlphabet(item.text).isEmpty())){
        LookAndFeel_V4::drawPopupMenuItemWithOptions(g,area,highlighted,item,options);return;
    }
    auto blank=item;blank.text={};LookAndFeel_V4::drawPopupMenuItemWithOptions(g,area,highlighted,blank,options);
    auto textArea=area.reduced(1);textArea.reduce(juce::jmin(5,area.getWidth()/20),0);
    const float iconWidth=textArea.getHeight()/1.3f;textArea.removeFromLeft(juce::roundToInt(iconWidth));
    if(item.image)textArea.removeFromLeft(juce::roundToInt(iconWidth*.5f));
    if(item.subMenu)textArea.removeFromRight(juce::roundToInt(.6f*getPopupMenuFont().getAscent()));
    textArea.removeFromRight(3);
    if(item.shortcutKeyDescription.isNotEmpty())textArea.removeFromRight(juce::roundToInt(getPopupMenuFont().getStringWidthFloat(item.shortcutKeyDescription)+8));
    juce::Component textStyle;textStyle.setEnabled(item.isEnabled);
    auto style=juce::var(new juce::DynamicObject);auto cfg=config.clone();
    const auto colour=highlighted&&item.isEnabled?findColour(juce::PopupMenu::highlightedTextColourId):item.colour!=juce::Colour()?item.colour:findColour(juce::PopupMenu::textColourId);
    cfg.getDynamicObject()->setProperty("textColour","#"+colour.toDisplayString(true));
    cfg.getDynamicObject()->setProperty("inactiveAlpha",0.);
    style.getDynamicObject()->setProperty("comboBoxLcd",cfg);textStyle.getProperties().set("silverSkinStyle",style);
    drawLcdText(g,textStyle,item.text,textArea.toFloat(),1,"comboBoxLcd");
}
void SkinLookAndFeel::getIdealPopupMenuItemSizeWithOptions(const juce::String& text,bool separator,int standardHeight,
        int& width,int& height,const juce::PopupMenu::Options& options)
{
    LookAndFeel_V4::getIdealPopupMenuItemSizeWithOptions(text,separator,standardHeight,width,height,options);
    if(const auto* combo=menuCombo(options)){
        const auto config=widgetStyle(*combo,"comboBoxLcd");
        if(!separator && bool(config["enabled"]) && bool(config["dotMatrixText"])){
            const float pitch=float(config.getProperty("dotSize",1.8));
            width=juce::jmax(width,juce::roundToInt(juce::jmin(80,lcdAlphabet(text).length())*6*pitch+50));
            height=juce::jmax(height,juce::roundToInt(10*pitch+4));
        }
    }
}
namespace {
// One shared painter, using precisely the R2 colours, line width and alpha.
void paintNavyLcd(juce::Graphics& g,float w,float h,const juce::var& config,bool enabled,bool focused)
{
    const auto top=config.hasProperty("topColour")?parseColour(config["topColour"]):juce::Colour(0xff112541);
    const auto bottom=config.hasProperty("bottomColour")?parseColour(config["bottomColour"]):juce::Colour(0xff061326);
    const auto grid=config.hasProperty("gridColour")?parseColour(config["gridColour"]):juce::Colour(0xff6aa3d9);
    const auto border=config.hasProperty("borderColour")?parseColour(config["borderColour"]):juce::Colour(0xff3c5878);
    const auto r=juce::Rectangle<float>(.5f,.5f,w-1,h-1);
    const juce::Graphics::ScopedSaveState save(g);
    g.setGradientFill(juce::ColourGradient(top,0,0,bottom,0,h,false));g.fillRoundedRectangle(r,3.f);
    {const juce::Graphics::ScopedSaveState clip(g);juce::Path p;p.addRoundedRectangle(r.reduced(1),2.f);g.reduceClipRegion(p);
     g.setColour(grid.withAlpha(float(config.getProperty("gridAlpha",.055))));
     const float pitch=float(config.getProperty("gridPitch",2.0));
     for(float x=1;x<w;x+=pitch)g.fillRect(x,1.f,.24f,h-2);
     for(float y=1;y<h;y+=pitch)g.fillRect(1.f,y,w-2,.24f);
    }
    g.setColour(enabled?border:border.withAlpha(.5f));g.drawRoundedRectangle(r,3.f,focused?1.5f:1.f);
}
}
void SkinLookAndFeel::drawPopupMenuBackgroundWithOptions(juce::Graphics& g,int w,int h,const juce::PopupMenu::Options& options)
{
    const auto* combo=menuCombo(options);
    const auto config=combo?widgetStyle(*combo,"comboBoxLcd"):package.theme["comboBoxLcd"];
    if(!bool(config["enabled"])){LookAndFeel_V4::drawPopupMenuBackgroundWithOptions(g,w,h,options);return;}
    g.fillAll(config.hasProperty("bottomColour")?parseColour(config["bottomColour"]):juce::Colour(0xff061326));
    paintNavyLcd(g,float(w),float(h),config,true,false);
}
bool SkinLookAndFeel::drawLcdComboBackground(juce::Graphics& g,const juce::ComboBox& c) const
{
    const auto config=widgetStyle(c,"comboBoxLcd");
    if(!bool(config["enabled"]) || bool(c.getProperties()["performancePreset"]))return false;
    paintNavyLcd(g,float(c.getWidth()),float(c.getHeight()),config,c.isEnabled(),c.hasKeyboardFocus(true));
    return true;
}

juce::Font SkinLookAndFeel::getLabelFont(juce::Label& l){return l.getFont();}
juce::Font SkinLookAndFeel::getComboBoxFont(juce::ComboBox&){return font(12.5f);}
juce::Font SkinLookAndFeel::getSliderPopupFont(juce::Slider&){return font(13.f);}
void SkinLookAndFeel::drawTooltip(juce::Graphics& g,const juce::String& text,int width,int height)
{
    if(width<2||height<2)return;
    const auto bounds=juce::Rectangle<float>(0,0,float(width),float(height));
    const auto outline=bounds.reduced(.5f);
    const float radius=juce::jmin(5.f,juce::jmin(outline.getWidth(),outline.getHeight())*.5f);
    juce::Path shape;shape.addRoundedRectangle(outline,radius);
    // Fill and outline share one contour; no dark corner can protrude past it.
    g.setColour(findColour(juce::TooltipWindow::backgroundColourId));g.fillPath(shape);
    g.setColour(findColour(juce::TooltipWindow::outlineColourId));g.strokePath(shape,juce::PathStrokeType(1.f));
    // Match pinned JUCE's tooltip typography and sizing algorithm exactly.
    juce::AttributedString content;content.setJustification(juce::Justification::centred);
    content.append(text,juce::Font(13.f,juce::Font::bold),findColour(juce::TooltipWindow::textColourId));
    juce::TextLayout layout;layout.createLayoutWithBalancedLineLengths(content,400.f);layout.draw(g,bounds);
}

bool SkinLookAndFeel::drawKnobStrip(juce::Graphics& g,juce::Slider& s,juce::Rectangle<float> r,float position)const
{
    const auto config=package.theme["knobStrip"];const auto style=s.getProperties()["silverSkinStyle"];
    const auto& bitmap=image(style.getProperty("knobStrip",config["image"]).toString());const int frames=int(config["frames"]);
    if(!bitmap.isValid() || frames<2)return false;
    const bool horizontal=config["orientation"].toString()=="horizontal";const int w=bitmap.getWidth()/(horizontal?frames:1),h=bitmap.getHeight()/(horizontal?1:frames),frame=juce::jlimit(0,frames-1,juce::roundToInt(position*(frames-1)));
    g.drawImage(bitmap,r.getX(),r.getY(),r.getWidth(),r.getHeight(),horizontal?frame*w:0,horizontal?0:frame*h,w,h);return true;
}
juce::Colour skinColour(const juce::Component& c,juce::uint32 value)
{
    if(const auto* l=look(c)){
        const auto palette=c.getProperties()["silverSkinStyle"]["palette"];const auto v=palette[juce::Identifier(paletteKey(value))];
        return colourString(v)?parseColour(v).withMultipliedAlpha(juce::Colour(value).getFloatAlpha()):l->colour(value);
    }
    return juce::Colour(value);
}
bool skinIsFlat(const juce::Component& c){const auto* l=look(c);return l && l->isFlat();}
void setSkinColour(juce::Component& c,int id,juce::Colour value)
{
    c.getProperties().set(juce::Identifier("silverSkinColour_"+juce::String::toHexString(id)),int(value.getARGB()));
    c.setColour(id,skinColour(c,value));
}
juce::Font skinFont(const juce::Component& c,float height,int style){
    const auto size=c.getProperties()["silverSkinStyle"]["fontSize"];if(number(size))height=float(size);
    if(const auto* l=look(c))return l->font(height,style);return juce::Font(height,style);
}
void applySkin(juce::Component& c,bool includeRoot)
{
    const auto* l=look(c);if(!l)return;
    if(includeRoot){
        const auto geometry=componentOptions(l->package.layout["components"],c);
        if(c.getComponentID()!="surface" && geometry.hasProperty("bounds"))c.setBounds(bounds(geometry["bounds"]));
        if(!bool(c.getProperties()["silverSkinApplied"])){
            const auto style=componentOptions(l->package.theme["components"],c);c.getProperties().set("silverSkinStyle",style);c.getProperties().set("silverSkinApplied",true);
            // Pinned JUCE stores explicit component colours in jcclr_<hex ID>.
            juce::Array<std::pair<int,juce::Colour>> updates;
            for(const auto& p:c.getProperties())if(p.name.toString().startsWith("jcclr_")){
                const auto id=p.name.toString().substring(6);const auto original=c.getProperties().getWithDefault(juce::Identifier("silverSkinColour_"+id),p.value);
                updates.add({id.getHexValue32(),skinColour(c,juce::uint32(juce::int64(original)))});
            }
            for(const auto& p:updates)c.setColour(p.first,p.second);
            for(const auto& p:l->package.theme["juceColours"].getDynamicObject()->getProperties()){
                const auto found=juceColourIds().find(p.name.toString());if(found!=juceColourIds().end()&&c.isColourSpecified(found->second))c.setColour(found->second,parseColour(p.value));
            }
            if(style.hasProperty("align"))if(auto* label=dynamic_cast<juce::Label*>(&c))label->setJustificationType(style["align"].toString()=="centre"?juce::Justification::centred:style["align"].toString()=="right"?juce::Justification::centredRight:juce::Justification::centredLeft);
            for(const auto& key:juce::StringArray{"rotaryStyle","rotaryDiameter","standKnob","blueVolume","captionWidth"})if(style.hasProperty(juce::Identifier(key)))c.getProperties().set(juce::Identifier(key),style[juce::Identifier(key)]);
        }
        const auto style=c.getProperties()["silverSkinStyle"];
        if(auto* label=dynamic_cast<juce::Label*>(&c)){
            auto& props=label->getProperties();
            if(!props.contains("silverSkinBaseFont"))props.set("silverSkinBaseFont",juce::var(new BaseFont(label->getFont())));
            const auto base=static_cast<BaseFont*>(props["silverSkinBaseFont"].getDynamicObject())->font;const float height=float(style.getProperty("fontSize",base.getHeight()));
            const auto f=l->package.typeface || l->package.theme["font"]["family"].toString().isNotEmpty()?l->font(height,base.getStyleFlags()):base.withHeight(height*float(l->package.theme["font"]["scale"]));
            if(label->getFont()!=f)label->setFont(f);
        }
        if(auto* editor=dynamic_cast<juce::TextEditor*>(&c)){
            auto& props=editor->getProperties();if(!props.contains("silverSkinBaseFont"))props.set("silverSkinBaseFont",juce::var(new BaseFont(editor->getFont())));
            const auto base=static_cast<BaseFont*>(props["silverSkinBaseFont"].getDynamicObject())->font;const auto f=l->font(float(style.getProperty("fontSize",base.getHeight())));if(editor->getFont()!=f)editor->setFont(f);
        }
    }
    for(auto* child:c.getChildren())applySkin(*child);
}
bool paintSkinBackground(const juce::Component& c,juce::Graphics& g)
{
    const auto* l=look(c);if(!l)return false;
    const auto style=c.getProperties()["silverSkinStyle"];const auto& image=l->image(style["backgroundImage"].toString());
    if(image.isValid()){g.drawImage(image,c.getLocalBounds().toFloat(),juce::RectanglePlacement::stretchToFit);return true;}
    return false;
}
bool skinDrawPanels(const juce::Component& c)
{
    const auto* l=look(c);return !l || (bool(l->package.layout.getProperty("drawPanels",true))&&!bool(c.getProperties()["silverSkinStyle"]["hideDecoration"]));
}
SkinDecoration::SkinDecoration(juce::var d):description(std::move(d))
{
    setComponentID("decoration:"+description["id"].toString());setInterceptsMouseClicks(false,false);setBounds(bounds(description["rect"]));
}
void SkinDecoration::paint(juce::Graphics& g)
{
    const auto* l=look(*this);if(!l)return;const auto kind=description["kind"].toString();
    if(kind=="image"){g.drawImage(l->image(description["image"].toString()),getLocalBounds().toFloat(),juce::RectanglePlacement::stretchToFit);return;}
    g.setColour(description.hasProperty("colour")?parseColour(description["colour"]):l->colour(0xff171b20));
    if(kind=="panel")g.fillRoundedRectangle(getLocalBounds().toFloat(),float(description.getProperty("radius",0.)));
    else{if(bool(description["lcdText"])&&l->drawLcdText(g,*this,description["text"].toString(),getLocalBounds().toFloat(),1))return;g.setFont(l->font(float(description.getProperty("fontSize",14.)),bool(description["bold"])?juce::Font::bold:juce::Font::plain));g.drawFittedText(description["text"].toString(),getLocalBounds(),description["align"].toString()=="centre"?juce::Justification::centred:description["align"].toString()=="right"?juce::Justification::centredRight:juce::Justification::centredLeft,4);}
}
}
