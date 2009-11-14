package org.omnetpp.common.wizard.support;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.FileDialog;
import org.eclipse.swt.widgets.Text;
import org.omnetpp.common.wizard.IWidgetAdapter;

/**
 * A control for selecting a file from the file system.
 * Implemented as a Composite with a single-line Text and a Browse button.
 * 
 * Does not replicate all the zillion methods of the Text class;
 * rather, it exposes the internal Text widget so that it can
 * be manipulated directly.
 * 
 * @author Andras
 */
public class ExternalFileChooser extends Composite implements IWidgetAdapter {
	private Text text;
	private Button browseButton;

	public ExternalFileChooser(Composite parent, int style) {
		super(parent, style);
		GridLayout layout = new GridLayout(2,false);
		layout.marginHeight = layout.marginWidth = 0;
		setLayout(layout);
		
		text = new Text(this, SWT.SINGLE|SWT.BORDER);
        text.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, true));
		browseButton = new Button(this, SWT.PUSH);
        browseButton.setText("Browse...");
		
		browseButton.addSelectionListener(new SelectionAdapter() {
			public void widgetSelected(SelectionEvent e) {
				browse();
			}
		});
	}

	protected void browse() {
        // filesystem selection:		
		FileDialog dialog = new FileDialog(getShell());
		dialog.setFileName(text.getText());
		String result = dialog.open();
		if (result != null) {
			text.setText(result);
	        text.selectAll();
		}
	}

	public String getFileName() {
		return text.getText();
	}

	public void setFileName(String file) {
		text.setText(file);
        text.selectAll();
	}

	public Text getTextControl() {
		return text;
	}

	public Button getBrowseButton() {
		return browseButton;
	}

	/**
	 * Adapter interface.
	 */
	public Object getValue() {
		return getFileName();
	}

	/**
	 * Adapter interface.
	 */
	public void setValue(Object value) {
		setFileName(value.toString());
	}

}
