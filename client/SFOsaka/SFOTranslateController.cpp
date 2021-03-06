#include "SFOTranslateController.h"

#include "SFOContext.h"
#include "SFOTranslateModel.h"

#include <QDebug>
#include <QQmlContext>
#include <QStringListModel>

#include <iostream>


const QString SFOTranslateController::ControllerIdentifier = "translateController";
const QString SFOTranslateController::ModelIdentifier = "translationModel";

SFOTranslateController::SFOTranslateController(QQmlContext *context,
                                               QObject *parent) :
    QObject(parent),
    _context(context),
    _noMatchesFound(false)
{
    _context->setContextProperty(ControllerIdentifier, this);
    _context->setContextProperty(ModelIdentifier, QVariant());

    SFOContext* ctx = SFOContext::GetInstance();
    // Listen for the partners updated signal so we can updated ourselves
    QObject::connect(ctx, &SFOContext::DictionariesUpdated,
                     this, &SFOTranslateController::_HandleDictionariesUpdate);

}

SFOTranslateController::~SFOTranslateController()
{
    // nothing now
}

void
SFOTranslateController::AddValidator(const QVariant& ,
                                     SFOValidator *validator)
{
    // We don't need the identifier, because we only care about one text field
    // in this model
    _validator = validator;
    
}

QValidator::State
SFOTranslateController::Validate( const QVariant& , QString & input, int & )
{
    _ProcessInput(input);
    return QValidator::Acceptable;
}

QString
SFOTranslateController::GetResultsText() const
{
    return _resultsText;
}

void
SFOTranslateController::OnInputAccepted(const QString& text)
{
    _ProcessInput(text);
}

void
SFOTranslateController::OnFilterAccepted(const QString& text)
{
    _ProcessInput(text);
}

void
SFOTranslateController::_HandleDictionariesUpdate()
{
}

void
SFOTranslateController::_ProcessInput(const QString& text)
{
    qDebug() << "Input: " << text;
    // Gets the first character to see if we're in Japanese or English.
    SFOInputLanguage lang = SFOTypes::GetInputLanguage(text);
    SFOTranslateModel *model = new SFOTranslateModel();
    _translationModel = SFOTranslateModelSharedPtr(model);
    QPairMap translations;
    if (lang != SFOInvalidInput) {
        QString lowered = text.toLower();
        SFOContext *context = SFOContext::GetInstance();
        if (lang == SFOJapaneseInput) {
            translations = _GetMatch(lowered, context->GetJpToEnDict());
        } else if (lang == SFOEnglishInput) {
            QPairMap enToJp = context->GetEnToJpDict();
            translations = _GetMatch(lowered, enToJp);
            // We're going backwards - finding the japanese word that matches
            // this phonetic japanese word, and it's the english dictionary that
            // contains the phonetics, not the japanese to english.
            _AppendPhoneticJpMatch(translations, lowered, enToJp);
        }
    }
    bool noMatchFound = !text.isEmpty() and translations.empty();
    if (noMatchFound != _noMatchesFound) {
        _noMatchesFound = noMatchFound;
        // Changed since last time. Update the text
        if (noMatchFound) {
            _resultsText = (tr("No Matches Found. Use 'Add' to enter a new phrase."));
        } else {
            _resultsText = "";
        }
        emit ResultsTextChanged();
    }

    model->SetTranslations(translations);
    _context->setContextProperty(ModelIdentifier,
                                 model);
}

QPairMap
SFOTranslateController::_GetMatch(const QString& str,
                                  const QPairMap& dict) const
{
    QPairMap matches;
    for (QPairMap::const_iterator mit = dict.constBegin() ;
         mit != dict.constEnd() ; ++mit) {
        // XXX - this is better done with a filter. Do we have that?
        if (mit.key().toLower().contains(str)) {
            matches[mit.key()] = mit.value();
        }
    }

    return matches;
}

void
SFOTranslateController::_AppendPhoneticJpMatch(QPairMap& current,
                                               const QString& str,
                                               const QPairMap& dict) const
{
    for (QPairMap::const_iterator mit = dict.constBegin() ;
         mit != dict.constEnd() ; ++mit) {
        QStringPair val = mit.value();
        if (val.second.toLower().contains(str)) {
            current[mit.key()] = mit.value();
        }
    }
}
