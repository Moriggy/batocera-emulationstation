#include "views/gamelist/GridGameListView.h"

#include "animations/LambdaAnimation.h"
#include "views/UIModeController.h"
#include "views/ViewController.h"
#include "CollectionSystemManager.h"
#include "Settings.h"
#include "SystemData.h"
#include "LocaleES.h"
#include "Window.h"
#include "guis/GuiGamelistOptions.h"

#ifdef _RPI_
#include "components/VideoPlayerComponent.h"
#endif
#include "components/VideoVlcComponent.h"

GridGameListView::GridGameListView(Window* window, FolderData* root, const std::shared_ptr<ThemeData>& theme, std::string themeName, Vector2f gridSize) :
	ISimpleGameListView(window, root),
	mGrid(window),
	mDescContainer(window), mDescription(window),
	mImage(nullptr), mVideo(nullptr), mMarquee(nullptr), mThumbnail(nullptr),
	mLblRating(window), mLblReleaseDate(window), mLblDeveloper(window), mLblPublisher(window),
	mLblGenre(window), mLblPlayers(window), mLblLastPlayed(window), mLblPlayCount(window),

	mRating(window), mReleaseDate(window), mDeveloper(window), mPublisher(window),
	mGenre(window), mPlayers(window), mLastPlayed(window), mPlayCount(window),
	mName(window)
{
	setTag("grid");

	const float padding = 0.01f;

	mGrid.setGridSizeOverride(gridSize);
	mGrid.setPosition(mSize.x() * 0.1f, mSize.y() * 0.1f);
	mGrid.setDefaultZIndex(20);
	mGrid.setCursorChangedCallback([&](const CursorState& /*state*/) { updateInfoPanel(); });
	addChild(&mGrid);

	// metadata labels + values
	mLblRating.setText(_("Rating") + std::string(": "));
	addChild(&mLblRating);
	addChild(&mRating);
	mLblReleaseDate.setText(_("Released") + std::string(": "));
	addChild(&mLblReleaseDate);
	addChild(&mReleaseDate);
	mLblDeveloper.setText(_("Developer") + std::string(": "));
	addChild(&mLblDeveloper);
	addChild(&mDeveloper);
	mLblPublisher.setText(_("Publisher") + std::string(": "));
	addChild(&mLblPublisher);
	addChild(&mPublisher);
	mLblGenre.setText(_("Genre") + std::string(": "));
	addChild(&mLblGenre);
	addChild(&mGenre);
	mLblPlayers.setText(_("Players") + std::string(": "));
	addChild(&mLblPlayers);
	addChild(&mPlayers);
	mLblLastPlayed.setText(_("Last played") + std::string(": "));
	addChild(&mLblLastPlayed);
	mLastPlayed.setDisplayRelative(true);
	addChild(&mLastPlayed);
	mLblPlayCount.setText(_("Times played") + std::string(": "));
	addChild(&mLblPlayCount);
	addChild(&mPlayCount);

	mName.setPosition(mSize.x(), mSize.y());
	mName.setDefaultZIndex(40);
	mName.setColor(0xAAAAAAFF);
	mName.setFont(Font::get(FONT_SIZE_MEDIUM));
	mName.setHorizontalAlignment(ALIGN_CENTER);
	addChild(&mName);

	mDescContainer.setPosition(mSize.x() * padding, mSize.y() * 0.65f);
	mDescContainer.setSize(mSize.x() * (0.50f - 2*padding), mSize.y() - mDescContainer.getPosition().y());
	mDescContainer.setAutoScroll(true);
	mDescContainer.setDefaultZIndex(40);
	addChild(&mDescContainer);

	mDescription.setFont(Font::get(FONT_SIZE_SMALL));
	mDescription.setSize(mDescContainer.getSize().x(), 0);
	mDescContainer.addChild(&mDescription);


	initMDLabels();
	initMDValues();

	if (!themeName.empty())
		setThemeName(themeName);

	setTheme(theme);

	populateList(mRoot->getChildrenListToDisplay());
	updateInfoPanel();
}

void GridGameListView::createVideo()
{
	if (mVideo != nullptr)
		return;

	const float padding = 0.01f;

	// video
// Create the correct type of video window
#ifdef _RPI_
	if (Settings::getInstance()->getBool("VideoOmxPlayer"))
		mVideo = new VideoPlayerComponent(mWindow, "");
	else
#endif
		mVideo = new VideoVlcComponent(mWindow, "");

	mVideo->setSnapshotSource(IMAGE);
	mVideo->setOrigin(0.5f, 0.5f);
	mVideo->setPosition(mSize.x() * 0.25f, mSize.y() * 0.4f);
	mVideo->setSize(mSize.x() * (0.5f - 2 * padding), mSize.y() * 0.4f);
	mVideo->setStartDelay(2000);
	mVideo->setDefaultZIndex(31);
	addChild(mVideo);
}

void GridGameListView::onShow()
{
	ISimpleGameListView::onShow();
	updateInfoPanel();
}


GridGameListView::~GridGameListView()
{
	if (mImage != nullptr)
		delete mImage;

	if (mThumbnail != nullptr)
		delete mThumbnail;

	if (mMarquee != nullptr)
		delete mMarquee;

	if (mVideo != nullptr)
		delete mVideo;
}

void GridGameListView::setThemeName(std::string name)
{
	ISimpleGameListView::setThemeName(name);
	mGrid.setThemeName(getName());
}

FileData* GridGameListView::getCursor()
{
	if (mGrid.size() == 0)
		return nullptr;

	return mGrid.getSelected();
}

void GridGameListView::setCursor(FileData* file)
{
	if (!mGrid.setCursor(file) && file->getParent() != nullptr)
	{
		populateList(file->getParent()->getChildrenListToDisplay());
		mGrid.setCursor(file);
	}
}

std::string GridGameListView::getQuickSystemSelectRightButton()
{
	return "r2"; //rightshoulder
	//return "pagedown"; //rightshoulder
}

std::string GridGameListView::getQuickSystemSelectLeftButton()
{
	return "l2"; //leftshoulder
	//return "pageup"; //leftshoulder
}

bool GridGameListView::input(InputConfig* config, Input input)
{
	if (!UIModeController::getInstance()->isUIModeKid() && config->isMappedTo("select", input) && input.value)
	{
		Sound::getFromTheme(mTheme, getName(), "menuOpen")->play();
		mWindow->pushGui(new GuiGamelistOptions(mWindow, this->mRoot->getSystem(), true));
		return true;

		// Ctrl-R to reload a view when debugging
	}

	if(config->isMappedLike("left", input) || config->isMappedLike("right", input))
		return GuiComponent::input(config, input);

	return ISimpleGameListView::input(config, input);
}

const std::string GridGameListView::getImagePath(FileData* file)
{
	ImageSource src = mGrid.getImageSource();

	if (src == ImageSource::IMAGE)
		return file->getImagePath();
	else if (src == ImageSource::MARQUEE)
		return file->getMarqueePath();

	return file->getThumbnailPath();
}

void GridGameListView::populateList(const std::vector<FileData*>& files)
{
	mGrid.clear();
	mHeaderText.setText(mRoot->getSystem()->getFullName());
	if (files.size() > 0)
	{
		std::string systemName = mRoot->getSystem()->getFullName();

		bool favoritesFirst = Settings::getInstance()->getBool("FavoritesFirst");
		bool showFavoriteIcon = (systemName != "favorites");
		if (!showFavoriteIcon)
			favoritesFirst = false;

		if (favoritesFirst)
		{
			for (auto it = files.cbegin(); it != files.cend(); it++)
			{
				if (!(*it)->getFavorite())
					continue;

				if (showFavoriteIcon)
					mGrid.add(_U("\uF006 ") + (*it)->getName(), getImagePath(*it), (*it)->getVideoPath(), (*it)->getMarqueePath(), *it);
				else
					mGrid.add((*it)->getName(), getImagePath(*it), (*it)->getVideoPath(), (*it)->getMarqueePath(), *it);
			}
		}

		for (auto it = files.cbegin(); it != files.cend(); it++)
		{
			if ((*it)->getFavorite())
			{
				if (favoritesFirst)
					continue;

				if (showFavoriteIcon)
				{
					mGrid.add(_U("\uF006 ") + (*it)->getName(), getImagePath(*it), (*it)->getVideoPath(), (*it)->getMarqueePath(), *it);
					continue;
				}
			}

			mGrid.add((*it)->getName(), getImagePath(*it), (*it)->getVideoPath(), (*it)->getMarqueePath(), *it);
		}
	}
	else
	{
		addPlaceholder();
	}
}

void GridGameListView::onThemeChanged(const std::shared_ptr<ThemeData>& theme)
{
	ISimpleGameListView::onThemeChanged(theme);

	using namespace ThemeFlags;

	mGrid.applyTheme(theme, getName(), "gamegrid", ALL);
	mName.applyTheme(theme, getName(), "md_name", ALL);

	if (theme->getElement(getName(), "md_video", "video"))
	{
		createVideo();
		mVideo->applyTheme(theme, getName(), "md_video", ALL ^ (PATH));
	}
	else if (mVideo != nullptr)
	{
		removeChild(mVideo);
		delete mVideo;
		mVideo = nullptr;
	}

	if (theme->getElement(getName(), "md_image", "image"))
	{
		createImage();
		mImage->applyTheme(theme, getName(), "md_image", ALL ^ (PATH));
	}
	else if (mImage != nullptr)
	{
		removeChild(mImage);
		delete mImage;
		mImage = nullptr;
	}

	if (theme->getElement(getName(), "md_thumbnail", "image"))
	{
		createThumbnail();
		mThumbnail->applyTheme(theme, getName(), "md_thumbnail", ALL ^ (PATH));
	}
	else if (mThumbnail != nullptr)
	{
		removeChild(mThumbnail);
		delete mThumbnail;
		mThumbnail = nullptr;
	}

	if (theme->getElement(getName(), "md_marquee", "image"))
	{
		createMarquee();
		mMarquee->applyTheme(theme, getName(), "md_marquee", ALL ^ (PATH));
	}
	else if (mMarquee != nullptr)
	{
		removeChild(mMarquee);
		delete mMarquee;
		mMarquee = nullptr;
	}

	initMDLabels();
	std::vector<TextComponent*> labels = getMDLabels();
	assert(labels.size() == 8);
	const char* lblElements[8] = {
			"md_lbl_rating", "md_lbl_releasedate", "md_lbl_developer", "md_lbl_publisher",
			"md_lbl_genre", "md_lbl_players", "md_lbl_lastplayed", "md_lbl_playcount"
	};

	for(unsigned int i = 0; i < labels.size(); i++)
	{
		labels[i]->applyTheme(theme, getName(), lblElements[i], ALL);
	}


	initMDValues();
	std::vector<GuiComponent*> values = getMDValues();
	assert(values.size() == 8);
	const char* valElements[8] = {
			"md_rating", "md_releasedate", "md_developer", "md_publisher",
			"md_genre", "md_players", "md_lastplayed", "md_playcount"
	};

	for(unsigned int i = 0; i < values.size(); i++)
	{
		values[i]->applyTheme(theme, getName(), valElements[i], ALL ^ ThemeFlags::TEXT);
	}

	if (theme->getElement(getName(), "md_description", "text"))
	{
		mDescContainer.applyTheme(theme, getName(), "md_description", POSITION | ThemeFlags::SIZE | Z_INDEX);
		mDescription.setSize(mDescContainer.getSize().x(), 0);
		mDescription.applyTheme(theme, getName(), "md_description", ALL ^ (POSITION | ThemeFlags::SIZE | ThemeFlags::ORIGIN | TEXT | ROTATION));

		if (!isChild(&mDescContainer))
			addChild(&mDescContainer);
	}
	else
		removeChild(&mDescContainer);

	sortChildren();
	updateInfoPanel();
}

void GridGameListView::createImage()
{
	if (mImage != nullptr)
		return;

	const float padding = 0.01f;

	// Image
	mImage = new ImageComponent(mWindow);
	mImage->setAllowFading(false);
	mImage->setOrigin(0.5f, 0.5f);
	mImage->setPosition(mSize.x() * 0.25f, mGrid.getPosition().y() + mSize.y() * 0.2125f);
	mImage->setMaxSize(mSize.x() * (0.50f - 2 * padding), mSize.y() * 0.4f);
	mImage->setDefaultZIndex(30);
	addChild(mImage);
}

void GridGameListView::createThumbnail()
{
	if (mThumbnail != nullptr)
		return;

	const float padding = 0.01f;

	// Image
	mThumbnail = new ImageComponent(mWindow);
	mThumbnail->setAllowFading(false);
	mThumbnail->setOrigin(0.5f, 0.5f);
	mThumbnail->setPosition(mSize.x() * 0.25f, mGrid.getPosition().y() + mSize.y() * 0.2125f);
	mThumbnail->setMaxSize(mSize.x() * (0.50f - 2 * padding), mSize.y() * 0.4f);
	mThumbnail->setDefaultZIndex(30);
	addChild(mThumbnail);
}

void GridGameListView::createMarquee()
{
	const float padding = 0.01f;

	// Marquee
	mMarquee = new ImageComponent(mWindow);
	mMarquee->setAllowFading(false);
	mMarquee->setOrigin(0.5f, 0.5f);
	mMarquee->setPosition(mSize.x() * 0.25f, mSize.y() * 0.10f);
	mMarquee->setMaxSize(mSize.x() * (0.5f - 2 * padding), mSize.y() * 0.18f);
	mMarquee->setDefaultZIndex(35);
	addChild(mMarquee);
}

void GridGameListView::initMDLabels()
{
	std::vector<TextComponent*> components = getMDLabels();

	const unsigned int colCount = 2;
	const unsigned int rowCount = (int)(components.size() / 2);

	Vector3f start(mSize.x() * 0.01f, mSize.y() * 0.625f, 0.0f);

	const float colSize = (mSize.x() * 0.48f) / colCount;
	const float rowPadding = 0.01f * mSize.y();

	for(unsigned int i = 0; i < components.size(); i++)
	{
		const unsigned int row = i % rowCount;
		Vector3f pos(0.0f, 0.0f, 0.0f);
		if(row == 0)
		{
			pos = start + Vector3f(colSize * (i / rowCount), 0, 0);
		}else{
			// work from the last component
			GuiComponent* lc = components[i-1];
			pos = lc->getPosition() + Vector3f(0, lc->getSize().y() + rowPadding, 0);
		}

		components[i]->setFont(Font::get(FONT_SIZE_SMALL));
		components[i]->setPosition(pos);
		components[i]->setDefaultZIndex(40);
	}
}

void GridGameListView::initMDValues()
{
	std::vector<TextComponent*> labels = getMDLabels();
	std::vector<GuiComponent*> values = getMDValues();

	std::shared_ptr<Font> defaultFont = Font::get(FONT_SIZE_SMALL);
	mRating.setSize(defaultFont->getHeight() * 5.0f, (float)defaultFont->getHeight());
	mReleaseDate.setFont(defaultFont);
	mDeveloper.setFont(defaultFont);
	mPublisher.setFont(defaultFont);
	mGenre.setFont(defaultFont);
	mPlayers.setFont(defaultFont);
	mLastPlayed.setFont(defaultFont);
	mPlayCount.setFont(defaultFont);

	float bottom = 0.0f;

	const float colSize = (mSize.x() * 0.48f) / 2;
	for(unsigned int i = 0; i < labels.size(); i++)
	{
		const float heightDiff = (labels[i]->getSize().y() - values[i]->getSize().y()) / 2;
		values[i]->setPosition(labels[i]->getPosition() + Vector3f(labels[i]->getSize().x(), heightDiff, 0));
		values[i]->setSize(colSize - labels[i]->getSize().x(), values[i]->getSize().y());
		values[i]->setDefaultZIndex(40);

		float testBot = values[i]->getPosition().y() + values[i]->getSize().y();
		if(testBot > bottom)
			bottom = testBot;
	}

	mDescContainer.setPosition(mDescContainer.getPosition().x(), bottom + mSize.y() * 0.01f);
	mDescContainer.setSize(mDescContainer.getSize().x(), mSize.y() - mDescContainer.getPosition().y());
}

void GridGameListView::updateInfoPanel()
{
	FileData* file = (mGrid.size() == 0 || mGrid.isScrolling()) ? NULL : mGrid.getSelected();

	bool fadingOut;
	if(file == NULL)
	{
		if (mVideo != nullptr)
		{
			mVideo->setVideo("");
			mVideo->setImage("");
		}

		if (mImage != nullptr)
			mImage->setImage("");

		// mVideo->setImage("");
		// mDescription.setText("");
		fadingOut = true;
	}
	else
	{
		std::string imagePath = file->getImagePath().empty() ? file->getThumbnailPath() : file->getImagePath();

		if (mVideo != nullptr)
		{
			if (!mVideo->setVideo(file->getVideoPath()))
				mVideo->setDefaultVideo();

			std::string snapShot = imagePath;

			auto src = mVideo->getSnapshotSource();
			if (src == MARQUEE && !file->getMarqueePath().empty())
				snapShot = file->getMarqueePath();
			if (src == THUMBNAIL && !file->getThumbnailPath().empty())
				snapShot = file->getThumbnailPath();

			mVideo->setImage(snapShot);
		}

		if (mImage != nullptr)
			mImage->setImage(imagePath);

		if (mThumbnail != nullptr)
			mThumbnail->setImage(file->getThumbnailPath());

		if (mMarquee != nullptr)
			mMarquee->setImage(file->getMarqueePath(), false, mMarquee->getMaxSizeInfo());

		mDescription.setText(file->metadata.get("desc"));
		mDescContainer.reset();

		mRating.setValue(file->metadata.get("rating"));
		mReleaseDate.setValue(file->metadata.get("releasedate"));
		mDeveloper.setValue(file->metadata.get("developer"));
		mPublisher.setValue(file->metadata.get("publisher"));
		mGenre.setValue(file->metadata.get("genre"));
		mPlayers.setValue(file->metadata.get("players"));
		mName.setValue(file->metadata.get("name"));

		if(file->getType() == GAME)
		{
			mLastPlayed.setValue(file->metadata.get("lastplayed"));
			mPlayCount.setValue(file->metadata.get("playcount"));
		}

		fadingOut = false;
	}

	std::vector<GuiComponent*> comps = getMDValues();

	if (mVideo != nullptr)
		comps.push_back(mVideo);

	if (mMarquee != nullptr)
		comps.push_back(mMarquee);

	if (mThumbnail != nullptr)
		comps.push_back(mThumbnail);

	if (mImage != nullptr)
		comps.push_back(mImage);

	comps.push_back(&mDescription);
	comps.push_back(&mName);
	std::vector<TextComponent*> labels = getMDLabels();
	comps.insert(comps.cend(), labels.cbegin(), labels.cend());

	for(auto it = comps.cbegin(); it != comps.cend(); it++)
	{
		GuiComponent* comp = *it;
		// an animation is playing
		//   then animate if reverse != fadingOut
		// an animation is not playing
		//   then animate if opacity != our target opacity
		if((comp->isAnimationPlaying(0) && comp->isAnimationReversed(0) != fadingOut) ||
			(!comp->isAnimationPlaying(0) && comp->getOpacity() != (fadingOut ? 0 : 255)))
		{
			auto func = [comp](float t)
			{
				comp->setOpacity((unsigned char)(Math::lerp(0.0f, 1.0f, t)*255));
			};
			comp->setAnimation(new LambdaAnimation(func, 150), 0, nullptr, fadingOut);
		}
	}
}

void GridGameListView::addPlaceholder()
{
	// empty grid - add a placeholder
	FileData* placeholder = new FileData(PLACEHOLDER, "<No Entries Found>", this->mRoot->getSystem());
	mGrid.add(placeholder->getName(), "", "", "", placeholder);
}

void GridGameListView::launch(FileData* game)
{
	ViewController::get()->launch(game);
}

void GridGameListView::remove(FileData *game, bool deleteFile)
{
	if (deleteFile)
		Utils::FileSystem::removeFile(game->getPath());  // actually delete the file on the filesystem
	FolderData* parent = game->getParent();
	if (getCursor() == game)                     // Select next element in list, or prev if none
	{
		std::vector<FileData*> siblings = parent->getChildrenListToDisplay();
		auto gameIter = std::find(siblings.cbegin(), siblings.cend(), game);
		int gamePos = (int)std::distance(siblings.cbegin(), gameIter);
		if (gameIter != siblings.cend())
		{
			if ((gamePos + 1) < (int)siblings.size())
			{
				setCursor(siblings.at(gamePos + 1));
			} else if ((gamePos - 1) > 0) {
				setCursor(siblings.at(gamePos - 1));
			}
		}
	}
	mGrid.remove(game);
	if(mGrid.size() == 0)
	{
		addPlaceholder();
	}
	delete game;                                 // remove before repopulating (removes from parent)
	onFileChanged(parent, FILE_REMOVED);           // update the view, with game removed
}

void GridGameListView::onFileChanged(FileData* file, FileChangeType change)
{
	if (change == FILE_METADATA_CHANGED)
	{
		// might switch to a detailed view
		ViewController::get()->reloadGameListView(this);
		return;
	}

	ISimpleGameListView::onFileChanged(file, change);
}

std::vector<TextComponent*> GridGameListView::getMDLabels()
{
	std::vector<TextComponent*> ret;
	ret.push_back(&mLblRating);
	ret.push_back(&mLblReleaseDate);
	ret.push_back(&mLblDeveloper);
	ret.push_back(&mLblPublisher);
	ret.push_back(&mLblGenre);
	ret.push_back(&mLblPlayers);
	ret.push_back(&mLblLastPlayed);
	ret.push_back(&mLblPlayCount);
	return ret;
}

std::vector<GuiComponent*> GridGameListView::getMDValues()
{
	std::vector<GuiComponent*> ret;
	ret.push_back(&mRating);
	ret.push_back(&mReleaseDate);
	ret.push_back(&mDeveloper);
	ret.push_back(&mPublisher);
	ret.push_back(&mGenre);
	ret.push_back(&mPlayers);
	ret.push_back(&mLastPlayed);
	ret.push_back(&mPlayCount);
	return ret;
}

std::vector<HelpPrompt> GridGameListView::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts;

	if(Settings::getInstance()->getBool("QuickSystemSelect"))
		prompts.push_back(HelpPrompt("lr", _("SYSTEM"))); // batocera
	prompts.push_back(HelpPrompt("up/down/left/right", _("CHOOSE"))); // batocera
	prompts.push_back(HelpPrompt(BUTTON_OK, _("LAUNCH")));
	prompts.push_back(HelpPrompt(BUTTON_BACK, _("BACK")));
	if(!UIModeController::getInstance()->isUIModeKid())
		prompts.push_back(HelpPrompt("select", _("OPTIONS"))); // batocera
	if(mRoot->getSystem()->isGameSystem())
		prompts.push_back(HelpPrompt("x", _("RANDOM"))); // batocera
	if(mRoot->getSystem()->isGameSystem() && !UIModeController::getInstance()->isUIModeKid())
	{
		std::string prompt = CollectionSystemManager::get()->getEditingCollection();
		prompts.push_back(HelpPrompt("y", prompt));
	}
	return prompts;
}

// batocera
void GridGameListView::setCursorIndex(int cursor){
	mGrid.setCursorIndex(cursor);
}

// batocera
int GridGameListView::getCursorIndex(){
	return mGrid.getCursorIndex();
}

std::vector<FileData*> GridGameListView::getFileDataEntries()
{
	return mGrid.getObjects();
}
